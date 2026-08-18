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
    m_foliageRenderer = std::make_unique<FoliageRenderer>(context, m_swapchain->getRenderPass(), m_globalSetLayout.get());
    m_skyRenderer = std::make_unique<SkyRenderer>(context, m_swapchain->getRenderPass(), m_globalSetLayout.get());
    m_waterRenderer = std::make_unique<WaterRenderer>(context, m_swapchain->getRenderPass(), m_globalSetLayout.get());
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(m_device);
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

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &setLayout), "Failed to create global UBO set layout");
    m_globalSetLayout = vkh::DescriptorSetLayoutHandle(
        setLayout,
        [device = m_device](VkDescriptorSetLayout l) { vkDestroyDescriptorSetLayout(device, l, nullptr); }
    );
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

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &pool), "Failed to create global descriptor pool");
    m_globalDescriptorPool = vkh::DescriptorPoolHandle(
        pool,
        [device = m_device](VkDescriptorPool p) { vkDestroyDescriptorPool(device, p, nullptr); }
    );

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_globalSetLayout.get());
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
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
        m_globalSetLayout.get(),
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

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &layout), "Failed to create pipeline layout");
    m_pipelineLayout = vkh::PipelineLayoutHandle(
        layout,
        [device = m_device](VkPipelineLayout l) { vkDestroyPipelineLayout(device, l, nullptr); }
    );

    VkShaderModule vertShader = PipelineBuilder::createShaderModule(m_device, "shaders/terrain.vert.spv");
    VkShaderModule fragShader = PipelineBuilder::createShaderModule(m_device, "shaders/terrain.frag.spv");

    PipelineBuilder builder;
    builder.setVertexShader(vertShader)
        .setFragmentShader(fragShader)
        .setDepthState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setPipelineLayout(layout)
        .setRenderPass(m_swapchain->getRenderPass());

    m_solidPipeline = vkh::PipelineHandle(
        builder.build(m_device),
        [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
    );

    try {
        m_wireframePipeline = vkh::PipelineHandle(
            builder.setPolygonMode(VK_POLYGON_MODE_LINE).build(m_device),
            [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
        );
    } catch (const std::exception& e) {
        std::cerr << "Wireframe mode not supported: " << e.what() << std::endl;
        m_wireframePipeline = vkh::PipelineHandle();
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
        VkSemaphore semaphore;
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &semaphore), "Failed to create semaphore");
        m_imageAvailableSemaphores[i] = vkh::SemaphoreHandle(
            semaphore,
            [device = m_device](VkSemaphore s) { vkDestroySemaphore(device, s, nullptr); }
        );

        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &semaphore), "Failed to create semaphore");
        m_renderFinishedSemaphores[i] = vkh::SemaphoreHandle(
            semaphore,
            [device = m_device](VkSemaphore s) { vkDestroySemaphore(device, s, nullptr); }
        );

        VkFence fence;
        VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &fence), "Failed to create fence");
        m_inFlightFences[i] = vkh::FenceHandle(
            fence,
            [device = m_device](VkFence f) { vkDestroyFence(device, f, nullptr); }
        );
    }
}

void Renderer::onWindowResize(GLFWwindow*) {
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

    Atmosphere::State sky = Atmosphere::compute(config.timeOfDay);

    ubo.sunDir = glm::vec4(sky.activeLightDir, sky.sinElevation);
    ubo.sunColor = glm::vec4(sky.sunColor, sky.dayFactor);
    ubo.skyColorZenith = glm::vec4(sky.skyZenith, sky.ambientIntensity);
    ubo.skyColorHorizon = glm::vec4(sky.skyHorizon, sky.starFactor);
    float maxFogDist = static_cast<float>(config.viewRadius) * CHUNK_SIZE * 0.95f;
    ubo.terrainParams = glm::vec4(config.waterHeight, config.fogDensity, time, config.debugMode);
    ubo.biomeParams = glm::vec4(config.amplitude, static_cast<float>(config.presetType), static_cast<float>(config.seed), maxFogDist);

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
    float panelW = 520.0f;
    float panelH = 180.0f;

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

    std::ostringstream ssWater;
    ssWater << std::fixed << std::setprecision(2);
    ssWater << "Water: " << (config.showWater ? "ON" : "OFF") << " (Elev: " << static_cast<int>(config.waterHeight) << "m, Waves: " << config.waveAmplitude << "m @ " << config.waveSpeed << "x) [G, B/N, O/P]";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 106.0f, 1.0f, ssWater.str(), glm::vec4(0.30f, 0.85f, 1.0f, 1.0f));

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
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 124.0f, 1.0f, ssModes.str(), glm::vec4(0.50f, 1.0f, 0.65f, 1.0f));

    std::ostringstream ssFoliage;
    int gridSide = config.viewRadius * 2 + 1;
    ssFoliage << "Foliage: " << (config.showFoliage ? "ON" : "OFF") << " (" << static_cast<int>(config.foliageDensity * 100.0f) << "%) [E, U/I] | View: " << static_cast<int>(config.getViewDistance() * 2.0f) << "m (" << gridSide << "x" << gridSide << ")";
    m_uiOverlay->drawText(panelX + 14.0f, panelY + 142.0f, 1.0f, ssFoliage.str(), glm::vec4(0.35f, 1.0f, 0.50f, 1.0f));

    float ctrlY = panelY + panelH + 12.0f;
    float ctrlH = 220.0f;

    m_uiOverlay->drawRect(panelX, ctrlY, panelW, ctrlH, glm::vec4(0.02f, 0.05f, 0.12f, 0.88f));
    m_uiOverlay->drawRect(panelX, ctrlY, panelW, 2.0f, glm::vec4(0.95f, 0.65f, 0.25f, 0.90f));

    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 10.0f, 1.15f, "CONTROLS & SHORTCUTS (Toggle 'H')", glm::vec4(1.0f, 0.75f, 0.30f, 1.0f));

    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 30.0f, 1.0f, "- [W/A/S/D] : Move Camera | [SHIFT] : Fast Boost", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 46.0f, 1.0f, "- [SPACE/E] : Fly Up | [CTRL/Q] : Fly Down", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 62.0f, 1.0f, "- [T] : Pause/Play Day-Night | [ [ / ] ] : Scrub Time", glm::vec4(1.0f, 0.85f, 0.35f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 78.0f, 1.0f, "- [1-5] : Presets (1:Mtn, 2:Hills, 3:Canyon, 4:Island, 5:Multi-Biome)", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 94.0f, 1.0f, "- [G] : Toggle Water | [B / N] : Wave Amp -/+ | [O / P] : Water Elev -/+", glm::vec4(0.30f, 0.85f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 110.0f, 1.0f, "- [E] : Toggle Foliage | [U / I] : Foliage Density -/+", glm::vec4(0.35f, 1.0f, 0.50f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 126.0f, 1.0f, "- [Z / X] : Amplitude -/+ | [C / V] : Frequency -/+", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 142.0f, 1.0f, "- [- / =] : View Dist -/+ | [L] : LOD | [J / K] : LOD -/+", glm::vec4(0.90f, 0.95f, 1.0f, 1.0f));
    m_uiOverlay->drawText(panelX + 14.0f, ctrlY + 158.0f, 1.0f, "- [M] : Shading Mode | [F] : Wireframe | [H] : Hide HUD", glm::vec4(0.60f, 0.80f, 1.0f, 1.0f));
}

bool Renderer::beginFrame(GLFWwindow* window, uint32_t& imageIndex) {
    VkFence fence = m_inFlightFences[m_currentFrame].get();
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        m_device,
        m_swapchain->getSwapchain(),
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame].get(),
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        m_swapchain->recreate(m_context, window);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    vkResetFences(m_device, 1, &fence);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer recording");

    return true;
}

void Renderer::recordPass(VkCommandBuffer cmd, uint32_t imageIndex, const Camera& camera,
                          ChunkManager& chunkManager, const TerrainConfig& config,
                          const HUDInfo& hudInfo, float time) {
    Atmosphere::State sky = Atmosphere::compute(config.timeOfDay);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color.float32[0] = sky.skyClearColor.r;
    clearValues[0].color.float32[1] = sky.skyClearColor.g;
    clearValues[0].color.float32[2] = sky.skyClearColor.b;
    clearValues[0].color.float32[3] = sky.skyClearColor.a;
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

    FrameContext frame;
    frame.commandBuffer = cmd;
    frame.frameIndex = m_currentFrame;
    frame.globalDescriptorSet = m_globalDescriptorSets[m_currentFrame];
    frame.camera = &camera;
    frame.config = &config;
    frame.time = time;
    frame.screenSize = glm::vec2(
        static_cast<float>(m_swapchain->getExtent().width),
        static_cast<float>(m_swapchain->getExtent().height)
    );

    m_skyRenderer->recordRenderCommands(frame);

    VkPipeline activePipeline = (config.wireframe && static_cast<bool>(m_wireframePipeline)) ? m_wireframePipeline.get() : m_solidPipeline.get();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout.get(),
        0,
        1,
        &m_globalDescriptorSets[m_currentFrame],
        0,
        nullptr
    );

    Frustum frustum = Frustum::fromViewProj(camera.getViewProjMatrix());
    chunkManager.recordRenderCommands(cmd, m_pipelineLayout.get(), frustum, config);

    if (config.showFoliage && !config.wireframe) {
        m_foliageRenderer->recordRenderCommands(frame);
    }

    if (config.showWater && !config.wireframe) {
        m_waterRenderer->recordRenderCommands(frame);
    }

    drawHUD(camera, config, hudInfo);
    m_uiOverlay->end(m_currentFrame);
    m_uiOverlay->recordCommands(cmd, m_currentFrame, frame.screenSize.x, frame.screenSize.y);

    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd), "Failed to end command buffer recording");
}

void Renderer::submitFrame(VkCommandBuffer cmd, uint32_t imageIndex, GLFWwindow* window) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame].get()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame].get()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_context.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame].get()), "Failed to submit draw command buffer");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {m_swapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(m_context.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        m_swapchain->recreate(m_context, window);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::renderFrame(
    GLFWwindow* window,
    const Camera& camera,
    ChunkManager& chunkManager,
    const TerrainConfig& config,
    const HUDInfo& hudInfo,
    float time
) {
    uint32_t imageIndex = 0;
    if (!beginFrame(window, imageIndex)) {
        return;
    }

    updateUBO(m_currentFrame, camera, config, time);

    FoliageKey key;
    key.seed = config.seed;
    key.centerChunkX = chunkManager.getCenterChunkX();
    key.centerChunkZ = chunkManager.getCenterChunkZ();
    key.radius = chunkManager.getRadius();
    key.showFoliage = config.showFoliage;
    key.presetType = config.presetType;
    key.waterHeight = config.waterHeight;
    key.foliageDensity = config.foliageDensity;

    if (!(key == m_lastFoliageKey)) {
        m_lastFoliageKey = key;
        m_foliageRenderer->updateInstances(m_context, chunkManager.getChunkOrigins(), config);
    }

    recordPass(m_commandBuffers[m_currentFrame], imageIndex, camera, chunkManager, config, hudInfo, time);
    submitFrame(m_commandBuffers[m_currentFrame], imageIndex, window);
}