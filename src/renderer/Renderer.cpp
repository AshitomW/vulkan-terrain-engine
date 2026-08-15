#include "renderer/Renderer.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

Renderer::Renderer(const VulkanContext& context, GLFWwindow* window, VkDescriptorSetLayout chunkSSBOSetLayout)
    : m_context(context), m_device(context.getDevice()) {
    m_swapchain = std::make_unique<VulkanSwapchain>(context, window);
    createGlobalDescriptorSetLayout();
    createGlobalDescriptorResources();
    createPipelines(chunkSSBOSetLayout);
    createCommandBuffers();
    createSyncObjects();

    m_uiOverlay = std::make_unique<UIOverlay>(context, m_swapchain->getRenderPass());
    m_foliageRenderer = std::make_unique<FoliageRenderer>(context, m_swapchain->getRenderPass(), m_globalSetLayout);
    m_skyRenderer = std::make_unique<SkyRenderer>(context, m_swapchain->getRenderPass(), m_globalSetLayout);
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(m_device);

    m_skyRenderer.reset();
    m_foliageRenderer.reset();
    m_uiOverlay.reset();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);

        if (m_uboMapped[i]) {
            m_uboBuffers[i].unmap();
        }
        m_uboBuffers[i].destroy();
    }

    if (m_solidPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_solidPipeline, nullptr);
    }
    if (m_wireframePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_wireframePipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }

    if (m_globalDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_globalDescriptorPool, nullptr);
    }
    if (m_globalSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_globalSetLayout, nullptr);
    }

    m_swapchain.reset();
}

void Renderer::createGlobalDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_globalSetLayout), "Failed to create global UBO set layout");
}

void Renderer::createGlobalDescriptorResources() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_globalDescriptorPool), "Failed to create global descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_globalSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_globalDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, m_globalDescriptorSets.data()), "Failed to allocate global descriptor sets");

    VkDeviceSize bufferSize = sizeof(GlobalUBO);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_uboBuffers[i].create(
            m_context,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_uboMapped[i] = m_uboBuffers[i].map();

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uboBuffers[i].getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(GlobalUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_globalDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }
}

void Renderer::createPipelines(VkDescriptorSetLayout chunkSSBOSetLayout) {
    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        m_globalSetLayout,
        chunkSSBOSetLayout
    };

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ChunkPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout), "Failed to create pipeline layout");

    VkShaderModule vertShader = VulkanPipeline::createShaderModule(m_device, "shaders/terrain.vert.spv");
    VkShaderModule fragShader = VulkanPipeline::createShaderModule(m_device, "shaders/terrain.frag.spv");

    VulkanPipeline::createTerrainGraphicsPipeline(
        m_device,
        m_swapchain->getRenderPass(),
        m_pipelineLayout,
        vertShader,
        fragShader,
        false,
        m_solidPipeline
    );

    try {
        VulkanPipeline::createTerrainGraphicsPipeline(
            m_device,
            m_swapchain->getRenderPass(),
            m_pipelineLayout,
            vertShader,
            fragShader,
            true,
            m_wireframePipeline
        );
    } catch (const std::exception& e) {
        std::cerr << "Wireframe mode not supported: " << e.what() << std::endl;
        m_wireframePipeline = VK_NULL_HANDLE;
    }

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void Renderer::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_context.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()), "Failed to allocate command buffers");
}

void Renderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]), "Failed to create semaphore");
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]), "Failed to create semaphore");
        VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]), "Failed to create fence");
    }
}

void Renderer::onWindowResize(GLFWwindow*  ) {
    m_framebufferResized = true;
}

float Renderer::getAspectRatio() const {
    VkExtent2D extent = m_swapchain->getExtent();
    if (extent.height == 0) return 1.0f;
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

void Renderer::updateUBO(uint32_t frameIndex, const Camera& camera, const TerrainConfig& config, float time) {
    GlobalUBO ubo{};
    ubo.viewProj = camera.getViewProjMatrix();
    ubo.view = camera.getViewMatrix();
    ubo.proj = camera.getProjMatrix();
    ubo.cameraPos = glm::vec4(camera.getPosition(), 1.0f);

    float t = config.timeOfDay;
    float sunAngle = ((t - 6.0f) / 12.0f) * 3.141592653589793f;
    float sinElev = std::sin(sunAngle);
    float cosElev = std::cos(sunAngle);

    glm::vec3 sunDir = glm::normalize(glm::vec3(cosElev * 0.80f, sinElev, 0.35f));
    glm::vec3 moonDir = glm::normalize(glm::vec3(-cosElev * 0.80f, -sinElev, -0.35f));

    glm::vec3 skyZenith;
    glm::vec3 skyHorizon;
    glm::vec3 sunColor;
    float ambientIntensity;
    float dayFactor;
    float starFactor;
    using namespace EngineConstants::Environment::SkyColors;

    auto smoothstep = [](float edge0, float edge1, float x) {
        float val = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return val * val * (3.0f - 2.0f * val);
    };

    if (sinElev >= 0.15f) {
        float f = smoothstep(0.15f, 0.70f, sinElev);
        skyZenith = glm::mix(SUNSET_ZENITH, NOON_ZENITH, f);
        skyHorizon = glm::mix(SUNSET_HORIZON, NOON_HORIZON, f);
        sunColor = glm::mix(SUNSET_SUN * SUNSET_SUN_INTENSITY, NOON_SUN * NOON_SUN_INTENSITY, f);
        ambientIntensity = glm::mix(SUNSET_AMBIENT, NOON_AMBIENT, f);
        dayFactor = 1.0f;
        starFactor = 0.0f;
    } else if (sinElev >= 0.0f) {
        float f = smoothstep(0.0f, 0.15f, sinElev);
        skyZenith = glm::mix(DUSK_ZENITH, SUNSET_ZENITH, f);
        skyHorizon = glm::mix(SUNSET_HORIZON, SUNSET_HORIZON, f);
        sunColor = glm::mix(DUSK_SUN * DUSK_SUN_INTENSITY, SUNSET_SUN * SUNSET_SUN_INTENSITY, f);
        ambientIntensity = glm::mix(DUSK_AMBIENT, SUNSET_AMBIENT, f);
        dayFactor = glm::mix(0.35f, 1.0f, f);
        starFactor = 0.0f;
    } else if (sinElev >= -0.18f) {
        float f = smoothstep(-0.18f, 0.0f, sinElev);
        skyZenith = glm::mix(NIGHT_ZENITH, DUSK_ZENITH, f);
        skyHorizon = glm::mix(NIGHT_HORIZON, DUSK_HORIZON, f);
        sunColor = glm::mix(NIGHT_MOON * NIGHT_MOON_INTENSITY, DUSK_SUN * DUSK_SUN_INTENSITY, f);
        ambientIntensity = glm::mix(NIGHT_AMBIENT, DUSK_AMBIENT, f);
        dayFactor = glm::mix(0.0f, 0.35f, f);
        starFactor = 1.0f - f;
    } else {
        skyZenith = NIGHT_ZENITH;
        skyHorizon = NIGHT_HORIZON;
        sunColor = NIGHT_MOON * NIGHT_MOON_INTENSITY;
        ambientIntensity = NIGHT_AMBIENT;
        dayFactor = 0.0f;
        starFactor = 1.0f;
    }

    glm::vec3 activeLightDir;
    if (sinElev >= -0.05f) {
        float blend = smoothstep(-0.05f, 0.10f, sinElev);
        activeLightDir = glm::normalize(glm::mix(moonDir, sunDir, blend));
    } else {
        activeLightDir = moonDir;
    }

    ubo.sunDir = glm::vec4(activeLightDir, sinElev);
    ubo.sunColor = glm::vec4(sunColor, dayFactor);
    ubo.skyColorZenith = glm::vec4(skyZenith, ambientIntensity);
    ubo.skyColorHorizon = glm::vec4(skyHorizon, starFactor);
    ubo.terrainParams = glm::vec4(config.waterHeight, config.fogDensity, time, config.debugMode);
    ubo.biomeParams = glm::vec4(config.amplitude, static_cast<float>(config.presetType), static_cast<float>(config.seed), 0.0f);

    std::memcpy(m_uboMapped[frameIndex], &ubo, sizeof(GlobalUBO));
}

void Renderer::drawHUD(const Camera& camera, const TerrainConfig& config, const HUDInfo& hudInfo) {
    m_uiOverlay->begin();

    if (!hudInfo.showHUD) {
        m_uiOverlay->drawRect(12, 12, 440, 28, glm::vec4(0.02f, 0.05f, 0.12f, 0.85f));
        m_uiOverlay->drawRect(12, 12, 440, 2, glm::vec4(0.2f, 0.6f, 1.0f, 0.9f));
        m_uiOverlay->drawText(20, 20, 1.0f, "[H] Show HUD | [T] Day/Night | [ [ / ] ] Scrub Time | [E] Foliage", glm::vec4(0.9f, 0.95f, 1.0f, 1.0f));
        return;
    }

    float panelX = 14.0f;
    float panelY = 14.0f;
    float panelW = 500.0f;
    float panelH = 162.0f;

    m_uiOverlay->drawRect(panelX, panelY, panelW, panelH, glm::vec4(0.02f, 0.05f, 0.12f, 0.88f));
    m_uiOverlay->drawRect(panelX, panelY, panelW, 3.0f, glm::vec4(0.20f, 0.65f, 1.0f, 0.95f));

    m_uiOverlay->drawText(panelX + 14.0f, panelY + 12.0f, 1.35f, "VULKAN PROCEDURAL TERRAIN ENGINE", glm::vec4(0.35f, 0.80f, 1.0f, 1.0f));

    std::ostringstream ssStats;
    ssStats << "FPS: " << static_cast<int>(hudInfo.fps) << " | Chunks: " << hudInfo.activeChunks << " | Mouse: " << (hudInfo.mouseCaptured ? "CAPTURED" : "UNLOCKED");
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 34.0f, 1.0f, ssStats.str(), glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));

    glm::vec3 pos = camera.getPosition();
    std::ostringstream ssPos;
    ssPos << std::fixed << std::setprecision(1);
    ssPos << "Camera: (" << pos.x << ", " << pos.y << ", " << pos.z << ") | Preset: " << hudInfo.presetName;
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 52.0f, 1.0f, ssPos.str(), glm::vec4(0.80f, 0.88f, 0.95f, 1.0f));

    int hours = static_cast<int>(config.timeOfDay);
    int minutes = static_cast<int>((config.timeOfDay - hours) * 60.0f);
    std::ostringstream ssTime;
    ssTime << "Time: " << std::setw(2) << std::setfill('0') << hours << ":"
           << std::setw(2) << std::setfill('0') << minutes;
    if (config.timeOfDay >= 5.0f && config.timeOfDay < 7.5f) ssTime << " (Sunrise)";
    else if (config.timeOfDay >= 7.5f && config.timeOfDay < 17.5f) ssTime << " (Day)";
    else if (config.timeOfDay >= 17.5f && config.timeOfDay < 20.0f) ssTime << " (Sunset)";
    else ssTime << " (Night / Stars)";
    ssTime << " [" << (config.timeCycleRunning ? "PLAY" : "PAUSED") << "] [T: Pause, [ / ]: Scrub]";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 70.0f, 1.0f, ssTime.str(), glm::vec4(1.0f, 0.85f, 0.35f, 1.0f));

    std::ostringstream ssConfig;
    ssConfig << "Seed: " << config.seed << " | Amp: " << config.amplitude << " (Z/X) | Freq: " << config.frequency << " (C/V)";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 88.0f, 1.0f, ssConfig.str(), glm::vec4(0.95f, 0.95f, 0.95f, 1.0f));

    const char* modeNames[] = {"Realistic Biomes & Shadows", "LOD Level Colors", "Surface Normals", "Slope Steepness"};
    std::string lodStr;
    if (config.isDynamicLOD()) {
        lodStr = "Auto Distance (0..3)";
    } else if (config.lodMode == 0) {
        lodStr = "Forced LOD 0 (65x65 Ultra)";
    } else if (config.lodMode == 1) {
        lodStr = "Forced LOD 1 (33x33 High)";
    } else if (config.lodMode == 2) {
        lodStr = "Forced LOD 2 (17x17 Medium)";
    } else {
        lodStr = "Forced LOD 3 (9x9 Low)";
    }

    std::ostringstream ssModes;
    ssModes << "LOD: " << lodStr << " [L, J/K] | Mode: " << modeNames[static_cast<int>(config.debugMode)] << " [M]";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 106.0f, 1.0f, ssModes.str(), glm::vec4(0.50f, 1.0f, 0.65f, 1.0f));

    std::ostringstream ssFoliage;
    int gridSide = config.viewRadius * 2 + 1;
    ssFoliage << "Foliage: " << (config.showFoliage ? "ON" : "OFF") << " (" << static_cast<int>(config.foliageDensity * 100.0f) << "%) [E, U/I] | View: " << static_cast<int>(config.getViewDistance() * 2.0f) << "m (" << gridSide << "x" << gridSide << ")";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 124.0f, 1.0f, ssFoliage.str(), glm::vec4(0.35f, 1.0f, 0.50f, 1.0f));

    float ctrlY = panelY + panelH + 12.0f;
    float ctrlH = 205.0f;

    m_uiOverlay->drawRect(panelX, ctrlY, panelW, ctrlH, glm::vec4(0.02f, 0.05f, 0.12f, 0.88f));
    m_uiOverlay->drawRect(panelX, ctrlY, panelW, 2.0f, glm::vec4(0.95f, 0.65f, 0.25f, 0.90f));

    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 10.0f, 1.15f, "CONTROLS & SHORTCUTS (Toggle 'H')", glm::vec4(1.0f, 0.75f, 0.30f, 1.0f));

    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 30.0f, 1.0f, "- [W/A/S/D] : Move Camera | [SHIFT] : Fast Boost", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 46.0f, 1.0f, "- [SPACE/E] : Fly Up | [CTRL/Q] : Fly Down", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 62.0f, 1.0f, "- [T] : Pause/Play Day-Night | [ [ / ] ] : Scrub Time", glm::vec4(1.0f, 0.85f, 0.35f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 78.0f, 1.0f, "- [1-5] : Presets (1:Mtn, 2:Hills, 3:Canyon, 4:Island, 5:Multi-Biome)", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 94.0f, 1.0f, "- [E] : Toggle Foliage | [U / I] : Foliage Density -/+", glm::vec4(0.35f, 1.0f, 0.50f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 110.0f, 1.0f, "- [O / P] : Water Height -/+ | [Z / X] : Amplitude -/+", glm::vec4(0.40f, 0.90f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 126.0f, 1.0f, "- [- / =] : View Dist -/+ | [L] : LOD | [J / K] : LOD -/+", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 142.0f, 1.0f, "- [M] : Shading Mode | [F] : Wireframe | [H] : Hide HUD", glm::vec4(0.60f, 0.80f, 1.0f, 1.0f));
}

void Renderer::renderFrame(
    GLFWwindow* window,
    const Camera& camera,
    ChunkManager& chunkManager,
    const TerrainConfig& config,
    const HUDInfo& hudInfo,
    float time
) {
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_device,
        m_swapchain->getSwapchain(),
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        m_swapchain->recreate(m_context, window);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    updateUBO(m_currentFrame, camera, config, time);

    if (config.seed != m_lastFoliageSeed ||
        chunkManager.getCenterChunkX() != m_lastCenterChunkX ||
        chunkManager.getCenterChunkZ() != m_lastCenterChunkZ ||
        chunkManager.getRadius() != m_lastRadius ||
        config.showFoliage != m_lastShowFoliage ||
        config.presetType != m_lastPresetType ||
        config.waterHeight != m_lastWaterHeight ||
        config.foliageDensity != m_lastFoliageDensity) {
        m_lastFoliageSeed = config.seed;
        m_lastCenterChunkX = chunkManager.getCenterChunkX();
        m_lastCenterChunkZ = chunkManager.getCenterChunkZ();
        m_lastRadius = chunkManager.getRadius();
        m_lastShowFoliage = config.showFoliage;
        m_lastPresetType = config.presetType;
        m_lastWaterHeight = config.waterHeight;
        m_lastFoliageDensity = config.foliageDensity;

        m_foliageRenderer->updateInstances(m_context, chunkManager.getChunkOrigins(), config);
    }

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer recording");

    float t = config.timeOfDay;
    float sunAngle = ((t - 6.0f) / 12.0f) * 3.141592653589793f;
    float sinElev = std::sin(sunAngle);

    VkClearColorValue skyClear;
    if (sinElev > 0.15f) {
        skyClear = {{0.025f, 0.090f, 0.280f, 1.0f}};
    } else if (sinElev > -0.15f) {
        float f = (sinElev + 0.15f) / 0.30f;
        skyClear = {{
            0.002f * (1.0f - f) + 0.025f * f,
            0.004f * (1.0f - f) + 0.090f * f,
            0.012f * (1.0f - f) + 0.280f * f,
            1.0f
        }};
    } else {
        skyClear = {{0.002f, 0.004f, 0.012f, 1.0f}};
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = skyClear;
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapchain->getRenderPass();
    renderPassInfo.framebuffer = m_swapchain->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->getExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->getExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchain->getExtent();
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    m_skyRenderer->recordRenderCommands(cmd, m_globalDescriptorSets[m_currentFrame]);

    VkPipeline activePipeline = (config.wireframe && m_wireframePipeline != VK_NULL_HANDLE) ? m_wireframePipeline : m_solidPipeline;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &m_globalDescriptorSets[m_currentFrame],
        0,
        nullptr
    );

    chunkManager.recordRenderCommands(cmd, m_pipelineLayout);

    if (config.showFoliage && !config.wireframe) {
        m_foliageRenderer->recordRenderCommands(cmd, m_pipelineLayout, m_globalDescriptorSets[m_currentFrame]);
    }

    drawHUD(camera, config, hudInfo);
    m_uiOverlay->end(m_currentFrame);
    m_uiOverlay->recordCommands(
        cmd,
        m_currentFrame,
        static_cast<float>(m_swapchain->getExtent().width),
        static_cast<float>(m_swapchain->getExtent().height)
    );

    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd), "Failed to end command buffer recording");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_context.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]), "Failed to submit draw command buffer");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {m_swapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_context.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        m_swapchain->recreate(m_context, window);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
