#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/VulkanImage.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "terrain/TerrainTypes.hpp"
#include <array>
#include <string_view>

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
    ~UIOverlay() = default;

    UIOverlay(const UIOverlay&) = delete;
    UIOverlay& operator=(const UIOverlay&) = delete;

    void begin();
    void drawRect(float x, float y, float w, float h, glm::vec4 color);
    void drawText(float x, float y, float scale, std::string_view text, glm::vec4 color);
    void end(uint32_t currentFrame);

    void recordCommands(VkCommandBuffer cmd, uint32_t currentFrame, float screenWidth, float screenHeight);

private:
    void createFontTexture(const VulkanContext& context);
    void createDescriptorResources(const VulkanContext& context);
    void createPipeline(const VulkanContext& context, VkRenderPass renderPass);
    void createVertexBuffers(const VulkanContext& context);

private:
    VkDevice m_device = VK_NULL_HANDLE;

    VulkanImage m_fontImage;
    vkh::SamplerHandle m_fontSampler;

    vkh::DescriptorSetLayoutHandle m_descriptorSetLayout;
    vkh::DescriptorPoolHandle m_descriptorPool;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    vkh::PipelineLayoutHandle m_pipelineLayout;
    vkh::PipelineHandle m_pipeline;

    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> m_vertexBuffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_vertexMapped{};
    std::vector<UIVertex> m_vertices;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> m_vertexCounts{};
};