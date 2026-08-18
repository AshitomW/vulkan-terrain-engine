#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "renderer/FrameContext.hpp"
#include <array>
#include <vector>
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
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    FoliageRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    ~FoliageRenderer();

    FoliageRenderer(const FoliageRenderer&) = delete;
    FoliageRenderer& operator=(const FoliageRenderer&) = delete;

    void updateInstances(
        const VulkanContext& context,
        const std::vector<glm::vec2>& chunkOrigins,
        const TerrainConfig& config
    );

    void recordRenderCommands(const FrameContext& frame);

private:
    void createModels(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout);
    void generateScatterForChunk(
        glm::vec2 chunkOrigin,
        const TerrainConfig& config,
        std::vector<FoliageInstance>& outInstances
    );

    float sampleExactHeight(glm::vec2 worldPos, const TerrainConfig& config) const;
    glm::vec3 sampleExactNormal(glm::vec2 worldPos, const TerrainConfig& config, float h) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    vkh::PipelineHandle m_pipeline;
    vkh::PipelineLayoutHandle m_pipelineLayout;

    VulkanBuffer m_vertexBuffer;
    VulkanBuffer m_indexBuffer;

    std::vector<ModelMesh> m_modelMeshes;

    static constexpr size_t NUM_MODELS = 7;
    std::vector<std::vector<FoliageInstance>> m_instancesByType;
    std::array<std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT>, NUM_MODELS> m_instanceBuffers;
    std::array<uint32_t, NUM_MODELS> m_instanceCounts{};
};