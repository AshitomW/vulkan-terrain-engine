#pragma once

#include "core/VulkanContext.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "renderer/FrameContext.hpp"

class SkyRenderer {
public:
    SkyRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    ~SkyRenderer();

    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    void recordRenderCommands(const FrameContext& frame);

private:
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    vkh::PipelineHandle m_pipeline;
    vkh::PipelineLayoutHandle m_pipelineLayout;
};