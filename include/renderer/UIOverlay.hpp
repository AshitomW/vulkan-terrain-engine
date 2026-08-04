#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/VulkanPipeline.hpp"
#include "terrain/TerrainTypes.hpp"
#include <vector>
#include <string>
#include <array>

struct UIVertex {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
};

class UIOverlay {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr size_t MAX_VERTICES = 12000;

    UIOverlay(const VulkanContext& context, VkRenderPass renderPass);
    ~UIOverlay();

    UIOverlay(const UIOverlay&) = delete;
    UIOverlay& operator=(const UIOverlay&) = delete;

    void begin();
    void drawRect(float x, float y, float w, float h, glm::vec4 color);
    void drawText(float x, float y, float scale, const std::string& text, glm::vec4 color);
    void end(uint32_t currentFrame);

    void recordCommands(VkCommandBuffer cmd, uint32_t currentFrame, float screenWidth, float screenHeight);

private:
    void createFontTexture(const VulkanContext& context);
    void createDescriptorResources(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass);
    void createVertexBuffers(const VulkanContext& context);

private:
    VkDevice m_device = VK_NULL_HANDLE;

    VkImage m_fontImage = VK_NULL_HANDLE;
    VkDeviceMemory m_fontMemory = VK_NULL_HANDLE;
    VkImageView m_fontImageView = VK_NULL_HANDLE;
    VkSampler m_fontSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> m_vertexBuffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_vertexMapped{};
    std::vector<UIVertex> m_vertices;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> m_vertexCounts{};
};
