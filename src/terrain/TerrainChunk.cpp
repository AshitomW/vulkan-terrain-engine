#include "terrain/TerrainChunk.hpp"
#include <cmath>
#include <array>

TerrainChunk::TerrainChunk(const VulkanContext& context, int chunkX, int chunkZ, VkDescriptorPool descriptorPool, VkDescriptorSetLayout ssboSetLayout)
    : m_chunkX(chunkX), m_chunkZ(chunkZ) {
    m_worldPos = glm::vec2(static_cast<float>(chunkX) * CHUNK_SIZE, static_cast<float>(chunkZ) * CHUNK_SIZE);
    createSSBO(context);
    createDescriptorSet(context, descriptorPool, ssboSetLayout);
}

TerrainChunk::~TerrainChunk() {
    m_ssboBuffer.destroy();
}

TerrainChunk::TerrainChunk(TerrainChunk&& other) noexcept
    : m_chunkX(other.m_chunkX),
      m_chunkZ(other.m_chunkZ),
      m_worldPos(other.m_worldPos),
      m_lod(other.m_lod),
      m_needsRegeneration(other.m_needsRegeneration),
      m_ssboBuffer(std::move(other.m_ssboBuffer)),
      m_descriptorSet(other.m_descriptorSet) {
    other.m_descriptorSet = VK_NULL_HANDLE;
}

TerrainChunk& TerrainChunk::operator=(TerrainChunk&& other) noexcept {
    if (this != &other) {
        m_ssboBuffer = std::move(other.m_ssboBuffer);
        m_chunkX = other.m_chunkX;
        m_chunkZ = other.m_chunkZ;
        m_worldPos = other.m_worldPos;
        m_lod = other.m_lod;
        m_needsRegeneration = other.m_needsRegeneration;
        m_descriptorSet = other.m_descriptorSet;

        other.m_descriptorSet = VK_NULL_HANDLE;
    }
    return *this;
}

void TerrainChunk::createSSBO(const VulkanContext& context) {

    VkDeviceSize bufferSize = CHUNK_GRID_RES * CHUNK_GRID_RES * sizeof(glm::vec4);

    m_ssboBuffer.create(
        context,
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
}

void TerrainChunk::createDescriptorSet(const VulkanContext& context, VkDescriptorPool descriptorPool, VkDescriptorSetLayout ssboSetLayout) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &ssboSetLayout;

    VK_CHECK(vkAllocateDescriptorSets(context.getDevice(), &allocInfo, &m_descriptorSet), "Failed to allocate chunk SSBO descriptor set");

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_ssboBuffer.getBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = m_ssboBuffer.getSize();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void TerrainChunk::setCoord(int chunkX, int chunkZ) {
    if (m_chunkX != chunkX || m_chunkZ != chunkZ) {
        m_chunkX = chunkX;
        m_chunkZ = chunkZ;
        m_worldPos = glm::vec2(static_cast<float>(chunkX) * CHUNK_SIZE, static_cast<float>(chunkZ) * CHUNK_SIZE);
        m_needsRegeneration = true;
    }
}

float TerrainChunk::getDistanceTo(const glm::vec3& pos) const {
    glm::vec2 chunkCenter = m_worldPos + glm::vec2(CHUNK_SIZE * 0.5f, CHUNK_SIZE * 0.5f);
    float dx = chunkCenter.x - pos.x;
    float dz = chunkCenter.y - pos.z;
    return std::sqrt(dx * dx + dz * dz);
}

void TerrainChunk::updateLOD(const glm::vec3& cameraPos, int lodMode) {
    if (lodMode >= 0 && lodMode < static_cast<int>(NUM_LOD_LEVELS)) {
        m_lod = static_cast<uint32_t>(lodMode);
        return;
    }

    float dist = getDistanceTo(cameraPos);

    if (dist < 180.0f) {
        m_lod = 0;
    } else if (dist < 360.0f) {
        m_lod = 1;
    } else if (dist < 620.0f) {
        m_lod = 2;
    } else {
        m_lod = 3;
    }
}
