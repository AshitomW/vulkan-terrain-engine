#include "renderer/SkyRenderer.hpp"

SkyRenderer::SkyRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout)
    : m_device(context.getDevice()) {
    createPipeline(context, renderPass, uboSetLayout);
}

SkyRenderer::~SkyRenderer() = default;

void SkyRenderer::createPipeline(const VulkanContext&, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout) {
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &uboSetLayout;

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &layout), "Failed to create sky pipeline layout");
    m_pipelineLayout = vkh::PipelineLayoutHandle(
        layout,
        [device = m_device](VkPipelineLayout l) { vkDestroyPipelineLayout(device, l, nullptr); }
    );

    VkShaderModule vertShader = PipelineBuilder::createShaderModule(m_device, "shaders/sky.vert.spv");
    VkShaderModule fragShader = PipelineBuilder::createShaderModule(m_device, "shaders/sky.frag.spv");

    PipelineBuilder builder;
    builder.setVertexShader(vertShader)
        .setFragmentShader(fragShader)
        .setCullMode(VK_CULL_MODE_NONE)
        .setDepthState(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setPipelineLayout(layout)
        .setRenderPass(renderPass);

    m_pipeline = vkh::PipelineHandle(
        builder.build(m_device),
        [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
    );

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void SkyRenderer::recordRenderCommands(const FrameContext& frame) {
    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());

    vkCmdBindDescriptorSets(
        frame.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout.get(),
        0,
        1,
        &frame.globalDescriptorSet,
        0,
        nullptr
    );

    vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);
}