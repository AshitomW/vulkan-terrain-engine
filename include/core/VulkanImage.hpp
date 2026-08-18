#pragma once

#include "core/VulkanContext.hpp"

class VulkanImage {
public:
    VulkanImage() = default;

    VulkanImage(
        const VulkanContext& context,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspectMask
    );

    ~VulkanImage();

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VulkanImage(VulkanImage&& other) noexcept;
    VulkanImage& operator=(VulkanImage&& other) noexcept;

    void create(
        const VulkanContext& context,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImageAspectFlags aspectMask
    );
    void destroy();

    VkImage getImage() const { return m_image; }
    VkImageView getView() const { return m_view; }
    VkFormat getFormat() const { return m_format; }
    uint32_t getWidth() const { return m_extent.width; }
    uint32_t getHeight() const { return m_extent.height; }
    bool isValid() const { return m_image != VK_NULL_HANDLE; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{0, 0};
    VmaAllocator m_allocator = VK_NULL_HANDLE;
};