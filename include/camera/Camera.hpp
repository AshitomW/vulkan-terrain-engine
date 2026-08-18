#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "core/EngineConstants.hpp"

class Camera {
public:
    Camera(
        glm::vec3 position = EngineConstants::Camera::DEFAULT_POSITION,
        float fov = EngineConstants::Camera::FOV
    );

    void update(float deltaTime, GLFWwindow* window);
    void onMouseMove(double xpos, double ypos);
    void setAspectRatio(float aspect);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;
    glm::mat4 getViewProjMatrix() const;

    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getForward() const { return m_forward; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

    bool isMouseCaptured() const { return m_isMouseCaptured; }
    void setMouseCaptured(bool captured) {
        m_isMouseCaptured = captured;
        m_firstMouse = true;
    }
    void toggleMouseCapture() {
        m_isMouseCaptured = !m_isMouseCaptured;
        m_firstMouse = true;
    }

private:
    void updateVectors();

private:
    glm::vec3 m_position;
    glm::vec3 m_forward{EngineConstants::Camera::DEFAULT_FRONT};
    glm::vec3 m_up{EngineConstants::Camera::WORLD_UP};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw = -90.0f;
    float m_pitch = -20.0f;
    float m_fov = EngineConstants::Camera::FOV;
    float m_aspect = 16.0f / 9.0f;
    float m_near = EngineConstants::Camera::NEAR_PLANE;
    float m_far = EngineConstants::Camera::FAR_PLANE;

    float m_moveSpeed = EngineConstants::Camera::DEFAULT_SPEED;
    float m_mouseSensitivity = EngineConstants::Camera::MOUSE_SENSITIVITY;

    bool m_firstMouse = true;
    double m_lastX = 0.0;
    double m_lastY = 0.0;
    bool m_isMouseCaptured = false;
};
