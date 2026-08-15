#include "app/Application.hpp"
#include <iostream>
#include <iomanip>
#include <random>

Application::Application(int width, int height, const char* title)
    : m_width(width), m_height(height), m_title(title) {
    initWindow();

    m_context = std::make_unique<VulkanContext>(m_window);

    m_camera = std::make_unique<Camera>(glm::vec3(0.0f, 65.0f, 120.0f));
    m_camera->setMouseCaptured(true);

    m_chunkManager = std::make_unique<ChunkManager>(*m_context, m_config.viewRadius);

    m_renderer = std::make_unique<Renderer>(*m_context, m_window, m_chunkManager->getSSBOSetLayout());
    m_camera->setAspectRatio(m_renderer->getAspectRatio());

    m_chunkManager->regenerateAll(*m_context, m_config);

    printHelp();
}

Application::~Application() {
    m_renderer.reset();
    m_chunkManager.reset();
    m_camera.reset();
    m_context.reset();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

void Application::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetKeyCallback(m_window, keyCallback);
}

void Application::printHelp() {

}

void Application::framebufferResizeCallback(GLFWwindow* window, int  , int  ) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->m_renderer) {
        app->m_renderer->onWindowResize(window);
        app->m_camera->setAspectRatio(app->m_renderer->getAspectRatio());
    }
}

void Application::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app && app->m_camera) {
        app->m_camera->onMouseMove(xpos, ypos);
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int  ) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (action == GLFW_PRESS) {
        if (!app->m_camera->isMouseCaptured()) {
            app->m_camera->setMouseCaptured(true);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void Application::keyCallback(GLFWwindow* window, int key, int  , int action, int  ) {
    if (action != GLFW_PRESS) return;

    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    bool needsRegen = false;

    if (key == GLFW_KEY_ESCAPE) {
        if (app->m_camera->isMouseCaptured()) {
            app->m_camera->setMouseCaptured(false);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    } else if (key == GLFW_KEY_TAB) {
        bool captured = !app->m_camera->isMouseCaptured();
        app->m_camera->setMouseCaptured(captured);
        glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    } else if (key == GLFW_KEY_H) {
        app->m_showHUD = !app->m_showHUD;
    } else if (key == GLFW_KEY_R) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(1, 999999);
        app->m_config.seed = dis(gen);
        app->m_currentPreset = "Custom (Random)";
        needsRegen = true;
    } else if (key == GLFW_KEY_1) {
        app->m_config.applyMountains();
        app->m_currentPreset = "Mountains";
        needsRegen = true;
    } else if (key == GLFW_KEY_2) {
        app->m_config.applyHills();
        app->m_currentPreset = "Rolling Hills";
        needsRegen = true;
    } else if (key == GLFW_KEY_3) {
        app->m_config.applyCanyons();
        app->m_currentPreset = "Deep Canyons";
        needsRegen = true;
    } else if (key == GLFW_KEY_4) {
        app->m_config.applyIslands();
        app->m_currentPreset = "Tropical Islands";
        needsRegen = true;
    } else if (key == GLFW_KEY_5) {
        app->m_config.applyMultiBiome();
        app->m_currentPreset = "Multi-Biome World";
        needsRegen = true;
    } else if (key == GLFW_KEY_T) {
        app->m_config.toggleTimeCycle();
    } else if (key == GLFW_KEY_LEFT_BRACKET) {
        app->m_config.scrubTime(-0.5f);
    } else if (key == GLFW_KEY_RIGHT_BRACKET) {
        app->m_config.scrubTime(+0.5f);
    } else if (key == GLFW_KEY_U) {
        app->m_config.decreaseFoliageDensity();
    } else if (key == GLFW_KEY_I) {
        app->m_config.increaseFoliageDensity();
    } else if (key == GLFW_KEY_E) {
        app->m_config.toggleFoliage();
    } else if (key == GLFW_KEY_O) {
        app->m_config.decreaseWaterHeight();
        needsRegen = true;
    } else if (key == GLFW_KEY_P) {
        app->m_config.increaseWaterHeight();
        needsRegen = true;
    } else if (key == GLFW_KEY_Z) {
        app->m_config.amplitude = std::max(5.0f, app->m_config.amplitude - 5.0f);
        app->m_currentPreset = "Custom";
        needsRegen = true;
    } else if (key == GLFW_KEY_X) {
        app->m_config.amplitude += 5.0f;
        app->m_currentPreset = "Custom";
        needsRegen = true;
    } else if (key == GLFW_KEY_C) {
        app->m_config.frequency = std::max(0.2f, app->m_config.frequency - 0.2f);
        app->m_currentPreset = "Custom";
        needsRegen = true;
    } else if (key == GLFW_KEY_V) {
        app->m_config.frequency += 0.2f;
        app->m_currentPreset = "Custom";
        needsRegen = true;
    } else if (key == GLFW_KEY_L) {
        app->m_config.cycleLODMode();
    } else if (key == GLFW_KEY_J || key == GLFW_KEY_DOWN) {
        app->m_config.increaseLOD();
    } else if (key == GLFW_KEY_K || key == GLFW_KEY_UP) {
        app->m_config.decreaseLOD();
    } else if (key == GLFW_KEY_G) {
        app->m_config.toggleWater();
    } else if (key == GLFW_KEY_B) {
        app->m_config.decreaseWaveAmplitude();
    } else if (key == GLFW_KEY_N) {
        app->m_config.increaseWaveAmplitude();
    } else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_COMMA) {
        app->m_config.decreaseViewDistance();
        app->m_chunkManager->setRadius(*app->m_context, app->m_config.viewRadius, app->m_config);
    } else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_PERIOD) {
        app->m_config.increaseViewDistance();
        app->m_chunkManager->setRadius(*app->m_context, app->m_config.viewRadius, app->m_config);
    } else if (key == GLFW_KEY_M) {
        int currentMode = static_cast<int>(app->m_config.debugMode);
        currentMode = (currentMode + 1) % 4;
        app->m_config.debugMode = static_cast<float>(currentMode);
    } else if (key == GLFW_KEY_F) {
        app->m_config.wireframe = !app->m_config.wireframe;
    }

    if (needsRegen) {
        app->m_chunkManager->regenerateAll(*app->m_context, app->m_config);
    }
}

void Application::run() {
    m_lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(m_window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        m_config.advanceTime(deltaTime);

        m_fpsTimer += deltaTime;
        m_frameCount++;
        if (m_fpsTimer >= 1.0f) {
            m_currentFPS = static_cast<float>(m_frameCount) / m_fpsTimer;
            m_frameCount = 0;
            m_fpsTimer = 0.0f;
        }

        glfwPollEvents();

        m_camera->update(deltaTime, m_window);

        m_chunkManager->update(*m_context, m_camera->getPosition(), m_config);

        HUDInfo hudInfo{};
        hudInfo.fps = m_currentFPS;
        hudInfo.activeChunks = m_chunkManager->getChunkCount();
        hudInfo.showHUD = m_showHUD;
        hudInfo.mouseCaptured = m_camera->isMouseCaptured();
        hudInfo.presetName = m_currentPreset;

        m_renderer->renderFrame(
            m_window,
            *m_camera,
            *m_chunkManager,
            m_config,
            hudInfo,
            currentTime
        );
    }

    m_context->waitIdle();
}
