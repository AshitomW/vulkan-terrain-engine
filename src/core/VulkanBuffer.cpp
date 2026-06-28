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
    : m_device(other.m_device),
      m_buffer(other.m_buffer),
      m_memory(other.m_memory),
      m_size(other.m_size),
      m_mapped(other.m_mapped) {
    other.m_device = VK_NULL_HANDLE;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_mapped = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_mapped = other.m_mapped;

        other.m_device = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mapped = nullptr;
    }
    return *this;
}

void VulkanBuffer::create(const VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    destroy();

    m_device = context.getDevice();
    m_size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer), "Failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context.findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory), "Failed to allocate buffer memory");
    VK_CHECK(vkBindBufferMemory(m_device, m_buffer, m_memory, 0), "Failed to bind buffer memory");
}

void VulkanBuffer::destroy() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_mapped != nullptr) {
            unmap();
        }
        if (m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }
        m_size = 0;
        m_device = VK_NULL_HANDLE;
    }
}

void* VulkanBuffer::map() {
    if (m_mapped == nullptr && m_memory != VK_NULL_HANDLE) {
        VK_CHECK(vkMapMemory(m_device, m_memory, 0, m_size, 0, &m_mapped), "Failed to map buffer memory");
    }
    return m_mapped;
}

void VulkanBuffer::unmap() {
    if (m_mapped != nullptr && m_device != VK_NULL_HANDLE) {
        vkUnmapMemory(m_device, m_memory);
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

    context.endSingleTimeCommands(commandBuffer);
}
