#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "terrain/TerrainTypes.hpp"

class TerrainChunk {
public:
    TerrainChunk(const VulkanContext& context, int chunkX, int chunkZ, VkDescriptorPool descriptorPool, VkDescriptorSetLayout ssboSetLayout);
    ~TerrainChunk();

    TerrainChunk(const TerrainChunk&) = delete;
    TerrainChunk& operator=(const TerrainChunk&) = delete;

    TerrainChunk(TerrainChunk&& other) noexcept;
    TerrainChunk& operator=(TerrainChunk&& other) noexcept;

    void setCoord(int chunkX, int chunkZ);
    void updateLOD(const glm::vec3& cameraPos, int lodMode);

    int getChunkX() const { return m_chunkX; }
    int getChunkZ() const { return m_chunkZ; }
    glm::vec2 getWorldPos() const { return m_worldPos; }
    uint32_t getLOD() const { return m_lod; }
    void setLOD(uint32_t lod) { m_lod = lod; }

    bool needsRegeneration() const { return m_needsRegeneration; }
    void setRegenerated() { m_needsRegeneration = false; }
    void markDirty() { m_needsRegeneration = true; }

    VkBuffer getSSBOBuffer() const { return m_ssboBuffer.getBuffer(); }
    VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

    float getDistanceTo(const glm::vec3& pos) const;

private:
    void createSSBO(const VulkanContext& context);
    void createDescriptorSet(const VulkanContext& context, VkDescriptorPool descriptorPool, VkDescriptorSetLayout ssboSetLayout);

private:
    int m_chunkX = 0;
    int m_chunkZ = 0;
    glm::vec2 m_worldPos{0.0f, 0.0f};

    uint32_t m_lod = 0;
    bool m_needsRegeneration = true;

    VulkanBuffer m_ssboBuffer;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};
