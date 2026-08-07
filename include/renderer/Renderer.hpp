#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanSwapchain.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/VulkanPipeline.hpp"
#include "renderer/UIOverlay.hpp"
#include "renderer/FoliageRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "camera/Camera.hpp"
#include "terrain/ChunkManager.hpp"
#include "terrain/TerrainTypes.hpp"
#include <array>
#include <memory>

struct HUDInfo {
    float fps = 0.0f;
    size_t activeChunks = 0;
    bool showHUD = true;
    bool mouseCaptured = true;
    const char* presetName = "Custom";
};

class Renderer {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(const VulkanContext& context, GLFWwindow* window, VkDescriptorSetLayout chunkSSBOSetLayout);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void renderFrame(
        GLFWwindow* window,
        const Camera& camera,
        ChunkManager& chunkManager,
        const TerrainConfig& config,
        const HUDInfo& hudInfo,
        float time
    );

    void onWindowResize(GLFWwindow* window);
    float getAspectRatio() const;

private:
    void createGlobalDescriptorSetLayout();
    void createGlobalDescriptorResources();
    void createPipelines(VkDescriptorSetLayout chunkSSBOSetLayout);
    void createCommandBuffers();
    void createSyncObjects();
    void updateUBO(uint32_t currentFrame, const Camera& camera, const TerrainConfig& config, float time);
    void drawHUD(const Camera& camera, const TerrainConfig& config, const HUDInfo& hudInfo);

private:
    const VulkanContext& m_context;
    VkDevice m_device = VK_NULL_HANDLE;

    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<UIOverlay> m_uiOverlay;
    std::unique_ptr<FoliageRenderer> m_foliageRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;

    VkDescriptorSetLayout m_globalSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_globalDescriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_globalDescriptorSets{};

    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> m_uboBuffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_uboMapped{};

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_solidPipeline = VK_NULL_HANDLE;
    VkPipeline m_wireframePipeline = VK_NULL_HANDLE;

    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> m_renderFinishedSemaphores{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences{};

    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    uint32_t m_lastFoliageSeed = 0;
    int m_lastCenterChunkX = -9999;
    int m_lastCenterChunkZ = -9999;
    int m_lastRadius = -1;
    bool m_lastShowFoliage = true;
    uint32_t m_lastPresetType = 9999;
    float m_lastWaterHeight = -9999.0f;
    float m_lastFoliageDensity = -1.0f;
};
