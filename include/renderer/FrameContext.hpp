#pragma once

#include "core/VulkanContext.hpp"
#include "camera/Camera.hpp"
#include "terrain/TerrainTypes.hpp"

struct FrameContext {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint32_t frameIndex = 0;
    VkDescriptorSet globalDescriptorSet = VK_NULL_HANDLE;
    const Camera* camera = nullptr;
    const TerrainConfig* config = nullptr;
    float time = 0.0f;
    glm::vec2 screenSize{0.0f, 0.0f};
};