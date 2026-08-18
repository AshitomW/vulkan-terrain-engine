#pragma once

#include "core/VulkanContext.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "terrain/TerrainChunk.hpp"
#include "terrain/TerrainTypes.hpp"
#include <vector>

class ComputeTerrainGenerator {
public:
    ComputeTerrainGenerator(const VulkanContext& context, VkDescriptorSetLayout ssboSetLayout);
    ~ComputeTerrainGenerator() = default;

    ComputeTerrainGenerator(const ComputeTerrainGenerator&) = delete;
    ComputeTerrainGenerator& operator=(const ComputeTerrainGenerator&) = delete;

    void generateChunks(const VulkanContext& context, const std::vector<TerrainChunk*>& chunks, const TerrainConfig& config);

private:
    void createPipeline(VkDescriptorSetLayout ssboSetLayout);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    vkh::PipelineLayoutHandle m_pipelineLayout;
    vkh::PipelineHandle m_pipeline;
    std::vector<VkBufferMemoryBarrier> m_barriers;
};