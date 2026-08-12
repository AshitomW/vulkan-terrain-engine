#pragma once

#include "core/VulkanContext.hpp"
#include "terrain/ChunkManager.hpp"
#include "renderer/Renderer.hpp"
#include "camera/Camera.hpp"
#include "terrain/TerrainTypes.hpp"
#include <memory>
#include <string>

class Application {
public:
    Application(int width = 1280, int height = 720, const char* title = "Vulkan Modular Terrain Engine");
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();

private:
    void initWindow();
    void printHelp();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    int m_width;
    int m_height;
    const char* m_title;

    GLFWwindow* m_window = nullptr;

    std::unique_ptr<VulkanContext> m_context;
    std::unique_ptr<ChunkManager> m_chunkManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Camera> m_camera;

    TerrainConfig m_config;
    bool m_showHUD = true;
    const char* m_currentPreset = "Mountains";

    float m_lastFrameTime = 0.0f;
    float m_fpsTimer = 0.0f;
    int m_frameCount = 0;
    float m_currentFPS = 0.0f;
};
