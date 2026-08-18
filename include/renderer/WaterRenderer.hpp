#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "renderer/FrameContext.hpp"

struct WaterVertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

class WaterRenderer {
public:
    WaterRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    ~WaterRenderer();

    WaterRenderer(const WaterRenderer&) = delete;
    WaterRenderer& operator=(const WaterRenderer&) = delete;

    void recordRenderCommands(const FrameContext& frame);

private:
    void createMesh(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    vkh::PipelineHandle m_pipeline;
    vkh::PipelineLayoutHandle m_pipelineLayout;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
    uint32_t m_indexCount = 0;
};