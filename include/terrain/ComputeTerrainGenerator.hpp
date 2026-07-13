#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanPipeline.hpp"
#include "terrain/TerrainChunk.hpp"
#include "terrain/TerrainTypes.hpp"
#include <vector>

class ComputeTerrainGenerator {
public:
    ComputeTerrainGenerator(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout);
    ~ComputeTerrainGenerator();

    ComputeTerrainGenerator(const ComputeTerrainGenerator&) = delete;
    ComputeTerrainGenerator& operator=(const ComputeTerrainGenerator&) = delete;

    void generateChunks(const VulkanContext& context, const std::vector<TerrainChunk*>& chunks, const TerrainConfig& config);

    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

private:
    void createPipeline(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
