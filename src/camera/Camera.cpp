#include "camera/Camera.hpp"
#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position, float fov)
    : m_position(position), m_fov(fov), m_isMouseCaptured(true) {
    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_forward = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_forward, EngineConstants::Camera::WORLD_UP));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
}

void Camera::update(float deltaTime, GLFWwindow* window) {
    float speed = m_moveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speed *= EngineConstants::Camera::TURBO_MULTIPLIER;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        m_position += m_forward * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        m_position -= m_forward * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        m_position -= m_right * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        m_position += m_right * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        m_position += EngineConstants::Camera::WORLD_UP * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        m_position -= EngineConstants::Camera::WORLD_UP * speed;
    }
}

void Camera::onMouseMove(double xpos, double ypos) {
    if (!m_isMouseCaptured) {
        m_firstMouse = true;
        return;
    }

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - m_lastX) * m_mouseSensitivity;
    float yoffset = static_cast<float>(m_lastY - ypos) * m_mouseSensitivity;

    m_lastX = xpos;
    m_lastY = ypos;

    m_yaw += xoffset;
    m_pitch += yoffset;

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::setAspectRatio(float aspect) {
    m_aspect = aspect;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::getProjMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
    proj[1][1] *= -1.0f;
    return proj;
}

glm::mat4 Camera::getViewProjMatrix() const {
    return getProjMatrix() * getViewMatrix();
}
