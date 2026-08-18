#pragma once

#include "core/VulkanContext.hpp"
#include <functional>
#include <utility>

namespace vkh {

template <typename HandleT>
class VkResource {
public:
    using DestroyFn = std::function<void(HandleT)>;

    VkResource() = default;

    explicit VkResource(DestroyFn destroyFn)
        : m_destroy(std::move(destroyFn)) {}

    VkResource(HandleT handle, DestroyFn destroyFn)
        : m_handle(handle), m_destroy(std::move(destroyFn)) {}

    ~VkResource() {
        reset();
    }

    VkResource(const VkResource&) = delete;
    VkResource& operator=(const VkResource&) = delete;

    VkResource(VkResource&& other) noexcept
        : m_handle(std::exchange(other.m_handle, HandleT{})),
          m_destroy(std::move(other.m_destroy)) {}

    VkResource& operator=(VkResource&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = std::exchange(other.m_handle, HandleT{});
            m_destroy = std::move(other.m_destroy);
        }
        return *this;
    }

    VkResource& operator=(HandleT handle) {
        reset();
        m_handle = handle;
        return *this;
    }

    void reset() {
        if (m_handle != HandleT{} && m_destroy) {
            m_destroy(m_handle);
        }
        m_handle = HandleT{};
    }

    HandleT get() const { return m_handle; }
    explicit operator bool() const { return m_handle != HandleT{}; }

private:
    HandleT m_handle{};
    DestroyFn m_destroy;
};

using PipelineHandle = VkResource<VkPipeline>;
using PipelineLayoutHandle = VkResource<VkPipelineLayout>;
using RenderPassHandle = VkResource<VkRenderPass>;
using DescriptorSetLayoutHandle = VkResource<VkDescriptorSetLayout>;
using DescriptorPoolHandle = VkResource<VkDescriptorPool>;
using SemaphoreHandle = VkResource<VkSemaphore>;
using FenceHandle = VkResource<VkFence>;
using ImageViewHandle = VkResource<VkImageView>;
using SamplerHandle = VkResource<VkSampler>;
using ShaderModuleHandle = VkResource<VkShaderModule>;

}