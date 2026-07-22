#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "terrain/TerrainChunk.hpp"
#include "terrain/ComputeTerrainGenerator.hpp"
#include "terrain/TerrainTypes.hpp"
#include <vector>
#include <memory>
#include <array>

class ChunkManager {
public:
    ChunkManager(const VulkanContext& context, int radius = 3);
    ~ChunkManager();

    ChunkManager(const ChunkManager&) = delete;
    ChunkManager& operator=(const ChunkManager&) = delete;

    void update(const VulkanContext& context, const glm::vec3& cameraPos, const TerrainConfig& config);
    void regenerateAll(const VulkanContext& context, const TerrainConfig& config);
    void setRadius(const VulkanContext& context, int newRadius, const TerrainConfig& config);

    void recordRenderCommands(VkCommandBuffer commandBuffer, VkPipelineLayout graphicsPipelineLayout);

    VkDescriptorSetLayout getSSBOSetLayout() const { return m_ssboSetLayout; }
    size_t getChunkCount() const { return m_chunks.size(); }
    int getRadius() const { return m_radius; }
    int getCenterChunkX() const { return m_centerChunkX; }
    int getCenterChunkZ() const { return m_centerChunkZ; }

    std::vector<glm::vec2> getChunkOrigins() const {
        std::vector<glm::vec2> origins;
        origins.reserve(m_chunks.size());
        for (const auto& chunk : m_chunks) {
            origins.push_back(chunk->getWorldPos());
        }
        return origins;
    }

private:
    void createDescriptorResources(const VulkanContext& context);
    void createLODIndexBuffers(const VulkanContext& context);

    static uint32_t getLODGridRes(uint32_t lod) {

        uint32_t step = 1u << lod;
        return (CHUNK_GRID_RES - 1u) / step + 1u;
    }

    static uint32_t getLODStep(uint32_t lod) {
        return 1u << lod;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    int m_radius = 3;
    int m_centerChunkX = 0;
    int m_centerChunkZ = 0;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_ssboSetLayout = VK_NULL_HANDLE;

    std::unique_ptr<ComputeTerrainGenerator> m_generator;
    std::vector<std::unique_ptr<TerrainChunk>> m_chunks;

    std::array<VulkanBuffer, NUM_LOD_LEVELS> m_lodIndexBuffers;
    std::array<uint32_t, NUM_LOD_LEVELS> m_lodIndexCounts{};
};
