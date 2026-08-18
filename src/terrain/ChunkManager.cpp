#include "terrain/ChunkManager.hpp"
#include <cmath>

ChunkManager::ChunkManager(const VulkanContext& context, int radius)
    : m_device(context.getDevice()), m_radius(radius) {
    createDescriptorResources();
    createLODIndexBuffers(context);

    m_generator = std::make_unique<ComputeTerrainGenerator>(context, m_ssboSetLayout.get());

    rebuildChunks(context);
}

ChunkManager::~ChunkManager() = default;

void ChunkManager::createDescriptorResources() {
    VkDescriptorSetLayoutBinding ssboBinding{};
    ssboBinding.binding = 0;
    ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboBinding.descriptorCount = 1;
    ssboBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &ssboBinding;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &setLayout), "Failed to create SSBO set layout");
    m_ssboSetLayout = vkh::DescriptorSetLayoutHandle(
        setLayout,
        [device = m_device](VkDescriptorSetLayout l) { vkDestroyDescriptorSetLayout(device, l, nullptr); }
    );

    uint32_t maxPossibleChunks = 512;
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = maxPossibleChunks;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = maxPossibleChunks;

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &pool), "Failed to create chunk descriptor pool");
    m_descriptorPool = vkh::DescriptorPoolHandle(
        pool,
        [device = m_device](VkDescriptorPool p) { vkDestroyDescriptorPool(device, p, nullptr); }
    );
}

void ChunkManager::createLODIndexBuffers(const VulkanContext& context) {
    for (uint32_t lod = 0; lod < NUM_LOD_LEVELS; ++lod) {
        uint32_t gridRes = getLODGridRes(lod);
        uint32_t quads = gridRes - 1;

        std::vector<uint32_t> indices;
        indices.reserve(quads * quads * 6);

        for (uint32_t z = 0; z < quads; ++z) {
            for (uint32_t x = 0; x < quads; ++x) {
                uint32_t i0 = z * gridRes + x;
                uint32_t i1 = z * gridRes + (x + 1);
                uint32_t i2 = (z + 1) * gridRes + x;
                uint32_t i3 = (z + 1) * gridRes + (x + 1);

                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        m_lodIndexCounts[lod] = static_cast<uint32_t>(indices.size());
        VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

        VulkanBuffer stagingBuffer(
            context,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        stagingBuffer.copyFromHost(indices.data(), bufferSize);

        m_lodIndexBuffers[lod].create(
            context,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VulkanBuffer::copyBuffer(context, stagingBuffer.getBuffer(), m_lodIndexBuffers[lod].getBuffer(), bufferSize);
    }
}

void ChunkManager::rebuildChunks(const VulkanContext& context) {
    m_chunks.clear();
    m_chunks.reserve((2 * m_radius + 1) * (2 * m_radius + 1));

    for (int dz = -m_radius; dz <= m_radius; ++dz) {
        for (int dx = -m_radius; dx <= m_radius; ++dx) {
            m_chunks.emplace_back(
                context,
                m_centerChunkX + dx,
                m_centerChunkZ + dz,
                m_descriptorPool.get(),
                m_ssboSetLayout.get()
            );
        }
    }

    refreshChunkOrigins();
}

void ChunkManager::refreshChunkOrigins() {
    m_chunkOrigins.clear();
    m_chunkOrigins.reserve(m_chunks.size());
    for (const auto& chunk : m_chunks) {
        m_chunkOrigins.push_back(chunk.getWorldPos());
    }
}

void ChunkManager::setRadius(const VulkanContext& context, int newRadius, const TerrainConfig& config) {
    if (newRadius < 1) newRadius = 1;
    if (newRadius > 8) newRadius = 8;
    if (m_radius == newRadius && !m_chunks.empty()) return;

    m_radius = newRadius;

    m_chunks.clear();

    if (m_descriptorPool) {
        vkResetDescriptorPool(m_device, m_descriptorPool.get(), 0);
    }

    rebuildChunks(context);

    regenerateAll(context, config);
}

void ChunkManager::update(const VulkanContext& context, const glm::vec3& cameraPos, const TerrainConfig& config) {
    int targetCenterX = static_cast<int>(std::floor(cameraPos.x / CHUNK_SIZE));
    int targetCenterZ = static_cast<int>(std::floor(cameraPos.z / CHUNK_SIZE));

    bool centerShifted = (targetCenterX != m_centerChunkX || targetCenterZ != m_centerChunkZ);
    m_centerChunkX = targetCenterX;
    m_centerChunkZ = targetCenterZ;

    if (centerShifted) {
        size_t idx = 0;
        for (int dz = -m_radius; dz <= m_radius; ++dz) {
            for (int dx = -m_radius; dx <= m_radius; ++dx) {
                m_chunks[idx].setCoord(m_centerChunkX + dx, m_centerChunkZ + dz);
                idx++;
            }
        }
        refreshChunkOrigins();
    }

    m_dirtyChunks.clear();
    for (auto& chunk : m_chunks) {
        chunk.updateLOD(cameraPos, config.lodMode);
        if (chunk.needsRegeneration()) {
            m_dirtyChunks.push_back(&chunk);
        }
    }

    if (!m_dirtyChunks.empty()) {
        m_generator->generateChunks(context, m_dirtyChunks, config);
    }
}

void ChunkManager::regenerateAll(const VulkanContext& context, const TerrainConfig& config) {
    m_dirtyChunks.clear();
    m_dirtyChunks.reserve(m_chunks.size());
    for (auto& chunk : m_chunks) {
        chunk.markDirty();
        m_dirtyChunks.push_back(&chunk);
    }
    m_generator->generateChunks(context, m_dirtyChunks, config);
}

void ChunkManager::recordRenderCommands(VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout,
                                        const Frustum& frustum, const TerrainConfig& config) {
    float minY = -0.5f * config.amplitude;
    float maxY = 1.5f * config.amplitude;

    for (uint32_t lod = 0; lod < NUM_LOD_LEVELS; ++lod) {
        VkBuffer indexBuffer = m_lodIndexBuffers[lod].getBuffer();
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        uint32_t lodGridRes = getLODGridRes(lod);
        uint32_t lodStep = getLODStep(lod);

        for (const auto& chunk : m_chunks) {
            if (chunk.getLOD() != lod) continue;

            glm::vec2 wp = chunk.getWorldPos();
            glm::vec3 min(wp.x, minY, wp.y);
            glm::vec3 max(wp.x + CHUNK_SIZE, maxY, wp.y + CHUNK_SIZE);
            if (!frustum.intersectsAABB(min, max)) continue;

            VkDescriptorSet ssboSet = chunk.getDescriptorSet();

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipelineLayout,
                1,
                1,
                &ssboSet,
                0,
                nullptr
            );

            ChunkPushConstants pc{};
            pc.chunkOffset = glm::vec4(
                wp.x,
                wp.y,
                CHUNK_CELL_SIZE,
                static_cast<float>(CHUNK_GRID_RES)
            );
            pc.lodParams = glm::uvec4(lodGridRes, lodStep, lod, 0);

            vkCmdPushConstants(
                commandBuffer,
                graphicsPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(ChunkPushConstants),
                &pc
            );

            vkCmdDrawIndexed(commandBuffer, m_lodIndexCounts[lod], 1, 0, 0, 0);
        }
    }
}