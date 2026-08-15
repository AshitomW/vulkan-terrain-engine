#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/VulkanPipeline.hpp"
#include "terrain/TerrainTypes.hpp"
#include <glm/glm.hpp>
#include <vector>

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

    void recordRenderCommands(
        VkCommandBuffer commandBuffer,
        VkDescriptorSet uboDescriptorSet,
        const TerrainConfig& config,
        float time,
        glm::vec3 cameraPos
    );

    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

private:
    void createMesh(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
    uint32_t m_indexCount = 0;
};
