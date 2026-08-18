#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanSwapchain.hpp"
#include "core/VulkanBuffer.hpp"
#include "core/PipelineBuilder.hpp"
#include "core/VulkanResource.hpp"
#include "core/Frustum.hpp"
#include "renderer/UIOverlay.hpp"
#include "renderer/FoliageRenderer.hpp"
#include "renderer/SkyRenderer.hpp"
#include "renderer/WaterRenderer.hpp"
#include "renderer/FrameContext.hpp"
#include "renderer/Atmosphere.hpp"
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

struct FoliageKey {
    uint32_t seed = 0;
    int centerChunkX = -9999;
    int centerChunkZ = -9999;
    int radius = -1;
    bool showFoliage = true;
    uint32_t presetType = 9999;
    float waterHeight = -9999.0f;
    float foliageDensity = -1.0f;

    bool operator==(const FoliageKey& o) const {
        return seed == o.seed &&
            centerChunkX == o.centerChunkX &&
            centerChunkZ == o.centerChunkZ &&
            radius == o.radius &&
            showFoliage == o.showFoliage &&
            presetType == o.presetType &&
            waterHeight == o.waterHeight &&
            foliageDensity == o.foliageDensity;
    }
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

    bool beginFrame(GLFWwindow* window, uint32_t& imageIndex);
    void recordPass(VkCommandBuffer cmd, uint32_t imageIndex, const Camera& camera,
                    ChunkManager& chunkManager, const TerrainConfig& config,
                    const HUDInfo& hudInfo, float time);
    void submitFrame(VkCommandBuffer cmd, uint32_t imageIndex, GLFWwindow* window);

    void drawHUD(const Camera& camera, const TerrainConfig& config, const HUDInfo& hudInfo);

private:
    const VulkanContext& m_context;
    VkDevice m_device = VK_NULL_HANDLE;

    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<UIOverlay> m_uiOverlay;
    std::unique_ptr<FoliageRenderer> m_foliageRenderer;
    std::unique_ptr<SkyRenderer> m_skyRenderer;
    std::unique_ptr<WaterRenderer> m_waterRenderer;

    vkh::DescriptorSetLayoutHandle m_globalSetLayout;
    vkh::DescriptorPoolHandle m_globalDescriptorPool;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_globalDescriptorSets{};

    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> m_uboBuffers;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_uboMapped{};

    vkh::PipelineLayoutHandle m_pipelineLayout;
    vkh::PipelineHandle m_solidPipeline;
    vkh::PipelineHandle m_wireframePipeline;

    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};
    std::array<vkh::SemaphoreHandle, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores;
    std::array<vkh::SemaphoreHandle, MAX_FRAMES_IN_FLIGHT> m_renderFinishedSemaphores;
    std::array<vkh::FenceHandle, MAX_FRAMES_IN_FLIGHT> m_inFlightFences;

    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;
    FoliageKey m_lastFoliageKey;
};