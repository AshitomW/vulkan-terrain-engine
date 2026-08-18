#include "core/VulkanImage.hpp"
#include <utility>

VulkanImage::VulkanImage(
    const VulkanContext& context,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask
) {
    create(context, width, height, format, usage, aspectMask);
}

VulkanImage::~VulkanImage() {
    destroy();
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
    : m_device(std::exchange(other.m_device, VK_NULL_HANDLE)),
      m_image(std::exchange(other.m_image, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_view(std::exchange(other.m_view, VK_NULL_HANDLE)),
      m_format(other.m_format),
      m_extent(other.m_extent),
      m_allocator(other.m_allocator) {
    other.m_format = VK_FORMAT_UNDEFINED;
    other.m_extent = {0, 0};
    other.m_allocator = VK_NULL_HANDLE;
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
        m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_view = std::exchange(other.m_view, VK_NULL_HANDLE);
        m_format = other.m_format;
        m_extent = other.m_extent;
        m_allocator = other.m_allocator;

        other.m_format = VK_FORMAT_UNDEFINED;
        other.m_extent = {0, 0};
        other.m_allocator = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanImage::create(
    const VulkanContext& context,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask
) {
    destroy();

    m_device = context.getDevice();
    m_allocator = context.getAllocator();
    m_format = format;
    m_extent = {width, height};

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VK_CHECK(
        vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr),
        "Failed to create image"
    );

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_view), "Failed to create image view");
}

void VulkanImage::destroy() {
    if (m_view != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_view, nullptr);
        m_view = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_allocator, m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
    m_format = VK_FORMAT_UNDEFINED;
    m_extent = {0, 0};
    m_allocator = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
}