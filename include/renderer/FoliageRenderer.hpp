#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/VulkanPipeline.hpp"
#include "terrain/TerrainTypes.hpp"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct FoliageVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
};

struct FoliageInstance {
    glm::vec4 posScale;
    glm::vec4 params;
};

struct ModelMesh {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

class FoliageRenderer {
public:
    FoliageRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    ~FoliageRenderer();

    FoliageRenderer(const FoliageRenderer&) = delete;
    FoliageRenderer& operator=(const FoliageRenderer&) = delete;

    void updateInstances(
        const VulkanContext& context,
        const std::vector<glm::vec2>& chunkOrigins,
        const TerrainConfig& config
    );

    void recordRenderCommands(
        VkCommandBuffer commandBuffer,
        VkPipelineLayout pipelineLayout,
        VkDescriptorSet uboDescriptorSet
    );

    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

private:
    void createModels(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    void generateScatterForChunk(
        glm::vec2 chunkOrigin,
        const TerrainConfig& config,
        std::vector<FoliageInstance>& outInstances
    );

    float sampleExactHeight(glm::vec2 worldPos, const TerrainConfig& config);
    glm::vec3 sampleExactNormal(glm::vec2 worldPos, const TerrainConfig& config, float h);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;
    uint32_t m_totalIndexCount = 0;

    std::vector<ModelMesh> m_modelMeshes;

    static constexpr size_t NUM_MODELS = 7;
    std::vector<std::vector<FoliageInstance>> m_instancesByType;
    std::vector<VulkanBuffer> m_instanceBuffers;
    std::vector<uint32_t> m_instanceCounts;
};
