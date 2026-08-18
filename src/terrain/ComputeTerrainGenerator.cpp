#include "terrain/ComputeTerrainGenerator.hpp"

ComputeTerrainGenerator::ComputeTerrainGenerator(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout)
    : m_device(context.getDevice()) {
    createPipeline(ssboSetLayout);
}

void ComputeTerrainGenerator::createPipeline(VkDescriptorSetLayout ssboSetLayout) {
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

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &layout), "Failed to create compute pipeline layout");
    m_pipelineLayout = vkh::PipelineLayoutHandle(
        layout,
        [device = m_device](VkPipelineLayout l) { vkDestroyPipelineLayout(device, l, nullptr); }
    );

    VkShaderModule compModule = PipelineBuilder::createShaderModule(m_device, "shaders/terrain.comp.spv");
    m_pipeline = vkh::PipelineHandle(
        PipelineBuilder::createComputePipeline(m_device, compModule, layout),
        [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
    );
    vkDestroyShaderModule(m_device, compModule, nullptr);
}

void ComputeTerrainGenerator::generateChunks(const VulkanContext& context, const std::vector<TerrainChunk*>& chunks, const TerrainConfig& config) {
    if (chunks.empty()) return;

    VkCommandBuffer commandBuffer = context.beginSingleTimeCommands();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.get());

    m_barriers.clear();
    m_barriers.reserve(chunks.size());

    for (TerrainChunk* chunk : chunks) {
        VkDescriptorSet dSet = chunk->getDescriptorSet();
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            m_pipelineLayout.get(),
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
            m_pipelineLayout.get(),
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
        m_barriers.push_back(barrier);

        chunk->setRegenerated();
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0,
        0, nullptr,
        static_cast<uint32_t>(m_barriers.size()), m_barriers.data(),
        0, nullptr
    );

    context.executeSingleTimeCommands(commandBuffer);
}