#pragma once

#include "core/VulkanContext.hpp"
#include "core/VulkanImage.hpp"
#include "core/VulkanResource.hpp"
#include <vector>

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain {
public:
    VulkanSwapchain(const VulkanContext& context, GLFWwindow* window);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    void recreate(const VulkanContext& context, GLFWwindow* window);

    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
    VkRenderPass getRenderPass() const { return m_renderPass.get(); }
    VkFramebuffer getFramebuffer(size_t index) const { return m_framebuffers[index]; }
    VkExtent2D getExtent() const { return m_extent; }
    VkFormat getImageFormat() const { return m_imageFormat; }
    size_t getImageCount() const { return m_images.size(); }

    static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    void createSwapchain(const VulkanContext& context, GLFWwindow* window);
    void createImageViews();
    void createRenderPass();
    void createDepthResources(const VulkanContext& context);
    void createFramebuffers();
    void cleanup();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    VkFormat findDepthFormat(const VulkanContext& context);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{0, 0};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;

    VulkanImage m_depthImage;
    vkh::RenderPassHandle m_renderPass;
};