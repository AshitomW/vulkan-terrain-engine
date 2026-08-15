#include "renderer/WaterRenderer.hpp"
#include <iostream>
#include <vector>
#include <cmath>

WaterRenderer::WaterRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout)
    : m_device(context.getDevice()) {
    createMesh(context);
    createPipeline(context, renderPass, uboSetLayout);
}

WaterRenderer::~WaterRenderer() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    m_indexBuffer.destroy();
    m_vertexBuffer.destroy();
}

void WaterRenderer::createMesh(const VulkanContext& context) {
    const uint32_t RES = EngineConstants::Water::WATER_GRID_RES;
    const float GRID_SPAN = 2400.0f;
    const float HALF_SPAN = GRID_SPAN * 0.5f;

    std::vector<WaterVertex> vertices;
    vertices.reserve(RES * RES);

    for (uint32_t y = 0; y < RES; ++y) {
        float fY = static_cast<float>(y) / static_cast<float>(RES - 1);
        float posZ = (fY - 0.5f) * GRID_SPAN;

        for (uint32_t x = 0; x < RES; ++x) {
            float fX = static_cast<float>(x) / static_cast<float>(RES - 1);
            float posX = (fX - 0.5f) * GRID_SPAN;

            WaterVertex v;
            v.pos = glm::vec3(posX, 0.0f, posZ);
            v.uv = glm::vec2(fX, fY);
            vertices.push_back(v);
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

void WaterRenderer::createPipeline(const VulkanContext&  , VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout) {
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

    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout), "Failed to create water pipeline layout");

    VkShaderModule vertShader = VulkanPipeline::createShaderModule(m_device, "shaders/water.vert.spv");
    VkShaderModule fragShader = VulkanPipeline::createShaderModule(m_device, "shaders/water.frag.spv");

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShader;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShader;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(WaterVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(WaterVertex, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(WaterVertex, uv)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create water graphics pipeline");

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void WaterRenderer::recordRenderCommands(
    VkCommandBuffer commandBuffer,
    VkDescriptorSet uboDescriptorSet,
    const TerrainConfig& config,
    float time,
    glm::vec3 cameraPos
) {
    if (!config.showWater) return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &uboDescriptorSet,
        0,
        nullptr
    );

    const uint32_t RES = EngineConstants::Water::WATER_GRID_RES;
    const float GRID_SPAN = 2400.0f;
    float cellSize = GRID_SPAN / static_cast<float>(RES - 1);
    float snappedX = std::floor(cameraPos.x / cellSize) * cellSize;
    float snappedZ = std::floor(cameraPos.z / cellSize) * cellSize;

    WaterPushConstants pc{};
    pc.waterParams1 = glm::vec4(config.waterHeight, config.waveAmplitude, config.waveSpeed, time);
    pc.waterParams2 = glm::vec4(config.waterClarity, 1.0f, config.frequency, config.debugMode);
    pc.gridCenter = glm::vec4(snappedX, snappedZ, GRID_SPAN, 0.0f);

    vkCmdPushConstants(
        commandBuffer,
        m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(WaterPushConstants),
        &pc
    );

    VkBuffer vBuffers[] = {m_vertexBuffer.getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
}
