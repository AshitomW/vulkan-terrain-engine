#include "renderer/WaterRenderer.hpp"
#include <cmath>

WaterRenderer::WaterRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout)
    : m_device(context.getDevice()) {
    createMesh(context);
    createPipeline(context, renderPass, uboSetLayout);
}

WaterRenderer::~WaterRenderer() = default;

void WaterRenderer::createMesh(const VulkanContext& context) {
    const uint32_t RES = EngineConstants::Water::WATER_GRID_RES;
    const float GRID_SPAN = 2400.0f;

    std::vector<WaterVertex> vertices;
    vertices.reserve(RES * RES);

    for (uint32_t y = 0; y < RES; ++y) {
        float fY = static_cast<float>(y) / static_cast<float>(RES - 1);
        float posZ = (fY - 0.5f) * GRID_SPAN;

        for (uint32_t x = 0; x < RES; ++x) {
            float fX = static_cast<float>(x) / static_cast<float>(RES - 1);
            float posX = (fX - 0.5f) * GRID_SPAN;

            vertices.push_back({glm::vec3(posX, 0.0f, posZ), glm::vec2(fX, fY)});
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve((RES - 1) * (RES - 1) * 6);

    for (uint32_t y = 0; y < RES - 1; ++y) {
        for (uint32_t x = 0; x < RES - 1; ++x) {
            uint32_t topLeft = y * RES + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (y + 1) * RES + x;
            uint32_t bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    m_indexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vSize = sizeof(WaterVertex) * vertices.size();
    VulkanBuffer vStaging(context, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vStaging.copyFromHost(vertices.data(), vSize);
    m_vertexBuffer.create(context, vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanBuffer::copyBuffer(context, vStaging.getBuffer(), m_vertexBuffer.getBuffer(), vSize);

    VkDeviceSize iSize = sizeof(uint32_t) * indices.size();
    VulkanBuffer iStaging(context, iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    iStaging.copyFromHost(indices.data(), iSize);
    m_indexBuffer.create(context, iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanBuffer::copyBuffer(context, iStaging.getBuffer(), m_indexBuffer.getBuffer(), iSize);
}

void WaterRenderer::createPipeline(const VulkanContext&, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout) {
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(WaterPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &uboSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pcRange;

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &layout), "Failed to create water pipeline layout");
    m_pipelineLayout = vkh::PipelineLayoutHandle(
        layout,
        [device = m_device](VkPipelineLayout l) { vkDestroyPipelineLayout(device, l, nullptr); }
    );

    VkShaderModule vertShader = PipelineBuilder::createShaderModule(m_device, "shaders/water.vert.spv");
    VkShaderModule fragShader = PipelineBuilder::createShaderModule(m_device, "shaders/water.frag.spv");

    std::vector<VkVertexInputBindingDescription> bindings = {
        {0, sizeof(WaterVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(WaterVertex, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(WaterVertex, uv)}
    };

    PipelineBuilder builder;
    builder.setVertexShader(vertShader)
        .setFragmentShader(fragShader)
        .setVertexInput(bindings, attributes)
        .setCullMode(VK_CULL_MODE_NONE)
        .setDepthState(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setBlendEnable(true)
        .setPipelineLayout(layout)
        .setRenderPass(renderPass);

    m_pipeline = vkh::PipelineHandle(
        builder.build(m_device),
        [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
    );

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void WaterRenderer::recordRenderCommands(const FrameContext& frame) {
    const TerrainConfig& config = *frame.config;
    if (!config.showWater) return;

    VkCommandBuffer cmd = frame.commandBuffer;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout.get(),
        0,
        1,
        &frame.globalDescriptorSet,
        0,
        nullptr
    );

    const uint32_t RES = EngineConstants::Water::WATER_GRID_RES;
    const float GRID_SPAN = 2400.0f;
    float cellSize = GRID_SPAN / static_cast<float>(RES - 1);
    const glm::vec3& camPos = frame.camera->getPosition();
    float snappedX = std::floor(camPos.x / cellSize) * cellSize;
    float snappedZ = std::floor(camPos.z / cellSize) * cellSize;

    WaterPushConstants pc{};
    pc.waterParams1 = glm::vec4(config.waterHeight, config.waveAmplitude, config.waveSpeed, frame.time);
    pc.waterParams2 = glm::vec4(config.waterClarity, 1.0f, config.frequency, config.debugMode);
    pc.gridCenter = glm::vec4(snappedX, snappedZ, GRID_SPAN, 0.0f);

    vkCmdPushConstants(
        cmd,
        m_pipelineLayout.get(),
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(WaterPushConstants),
        &pc
    );

    VkBuffer vBuffers[] = {m_vertexBuffer.getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);
}