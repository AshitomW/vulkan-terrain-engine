#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullability-completeness"
#pragma GCC diagnostic ignored "-Wunused-private-field"
#include "core/vk_mem_alloc.h"
#pragma GCC diagnostic pop

#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <iostream>

#define VK_CHECK(result, msg) \
    do { \
        VkResult err = (result); \
        if (err != VK_SUCCESS) { \
            throw std::runtime_error(std::string(msg) + " (VkResult: " + std::to_string(err) + ")"); \
        } \
    } while (0)

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanContext {
public:
    VulkanContext(GLFWwindow* window, bool enableValidationLayers = true);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VulkanContext(VulkanContext&&) noexcept = default;
    VulkanContext& operator=(VulkanContext&&) noexcept = default;

    VkInstance getInstance() const { return m_instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getDevice() const { return m_device; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getComputeQueue() const { return m_computeQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkCommandPool getCommandPool() const { return m_commandPool; }
    VmaAllocator getAllocator() const { return m_allocator; }
    const QueueFamilyIndices& getQueueFamilies() const { return m_queueIndices; }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    VkCommandBuffer beginSingleTimeCommands() const;
    void executeSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    void waitIdle() const;

private:
    void createInstance(bool enableValidationLayers);
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createAllocator();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    QueueFamilyIndices m_queueIndices;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    bool m_portabilitySubsetSupported = false;
};
