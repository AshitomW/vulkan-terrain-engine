#include "core/VulkanBuffer.hpp"
#include <cstring>
#include <utility>

VulkanBuffer::VulkanBuffer(const VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    create(context, size, usage, properties);
}

VulkanBuffer::~VulkanBuffer() {
    destroy();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
      m_allocation(std::exchange(other.m_allocation, VK_NULL_HANDLE)),
      m_allocator(std::exchange(other.m_allocator, VK_NULL_HANDLE)),
      m_size(other.m_size),
      m_mapped(std::exchange(other.m_mapped, nullptr)) {
    other.m_size = 0;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
        m_size = other.m_size;
        m_mapped = std::exchange(other.m_mapped, nullptr);

        other.m_size = 0;
    }
    return *this;
}

void VulkanBuffer::create(const VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    destroy();

    m_allocator = context.getAllocator();
    m_size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    }

    VmaAllocationInfo allocationInfo{};
    VK_CHECK(
        vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &allocationInfo),
        "Failed to create buffer"
    );
    m_mapped = allocationInfo.pMappedData;
}

void VulkanBuffer::destroy() {
    if (m_buffer != VK_NULL_HANDLE && m_allocator != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_allocator = VK_NULL_HANDLE;
        m_size = 0;
        m_mapped = nullptr;
    }
}

void* VulkanBuffer::map() {
    if (m_mapped == nullptr && m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        VK_CHECK(vmaMapMemory(m_allocator, m_allocation, &m_mapped), "Failed to map buffer memory");
    }
    return m_mapped;
}

void VulkanBuffer::unmap() {
    if (m_mapped != nullptr && m_allocation != VK_NULL_HANDLE) {
        vmaUnmapMemory(m_allocator, m_allocation);
        m_mapped = nullptr;
    }
}

void VulkanBuffer::copyFromHost(const void* data, VkDeviceSize size) {
    if (size > m_size) {
        throw std::runtime_error("copyFromHost: size exceeds buffer capacity!");
    }
    void* dst = map();
    std::memcpy(dst, data, static_cast<size_t>(size));
}

void VulkanBuffer::copyBuffer(const VulkanContext& context, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = context.beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    context.executeSingleTimeCommands(commandBuffer);
}