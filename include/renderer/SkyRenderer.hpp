#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanPipeline.hpp"

class SkyRenderer {
public:
    SkyRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    ~SkyRenderer();

    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    void recordRenderCommands(
        VkCommandBuffer commandBuffer,
        VkDescriptorSet uboDescriptorSet
    );

private:
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
