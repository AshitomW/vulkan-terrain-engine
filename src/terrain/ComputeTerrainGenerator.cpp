#include "terrain/ComputeTerrainGenerator.hpp"
#include <array>

ComputeTerrainGenerator::ComputeTerrainGenerator(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout)
    : m_device(context.getDevice()) {
    createPipeline(context, ssboSetLayout);
}

ComputeTerrainGenerator::~ComputeTerrainGenerator() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

void ComputeTerrainGenerator::createPipeline(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ComputePushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &ssboSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout), "Failed to create compute pipeline layout");

    VkShaderModule compModule = VulkanPipeline::createShaderModule(m_device, "shaders/terrain.comp.spv");
    VulkanPipeline::createComputePipeline(m_device, compModule, m_pipelineLayout, m_pipeline);
    vkDestroyShaderModule(m_device, compModule, nullptr);
}

void ComputeTerrainGenerator::generateChunks(const VulkanContext& context, const std::vector<TerrainChunk*>& chunks, const TerrainConfig& config) {
    if (chunks.empty()) return;

    VkCommandBuffer commandBuffer = context.beginSingleTimeCommands();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

    std::vector<VkBufferMemoryBarrier> barriers;
    barriers.reserve(chunks.size());

    for (TerrainChunk* chunk : chunks) {
        VkDescriptorSet dSet = chunk->getDescriptorSet();
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            m_pipelineLayout,
            0,
            1,
            &dSet,
            0,
            nullptr
        );

        ComputePushConstants pc{};
        pc.chunkOffset = glm::vec4(chunk->getWorldPos().x, chunk->getWorldPos().y, CHUNK_CELL_SIZE, static_cast<float>(CHUNK_GRID_RES));
        pc.noiseParams1 = glm::vec4(config.frequency, config.amplitude, config.warpStrength, config.mountainPower);
        pc.noiseParams2 = glm::vec4(config.octaves, config.lacunarity, config.persistence, config.waterHeight);
        pc.config = glm::uvec4(config.seed, CHUNK_GRID_RES, config.presetType, 0);

        vkCmdPushConstants(
            commandBuffer,
            m_pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(ComputePushConstants),
            &pc
        );

        uint32_t groupCount = (CHUNK_GRID_RES + 15) / 16;
        vkCmdDispatch(commandBuffer, groupCount, groupCount, 1);

        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = chunk->getSSBOBuffer();
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;
        barriers.push_back(barrier);

        chunk->setRegenerated();
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(barriers.size()), barriers.data(),
        0, nullptr
    );

    context.endSingleTimeCommands(commandBuffer);
}
