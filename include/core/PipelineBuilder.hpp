#pragma once

#include "core/VulkanContext.hpp"
#include <vector>
#include <string>

class PipelineBuilder {
public:
    PipelineBuilder& setVertexShader(VkShaderModule module);
    PipelineBuilder& setFragmentShader(VkShaderModule module);

    PipelineBuilder& setVertexInput(
        const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes
    );

    PipelineBuilder& setTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPolygonMode(VkPolygonMode mode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode);
    PipelineBuilder& setFrontFace(VkFrontFace frontFace);
    PipelineBuilder& setRasterizationSamples(VkSampleCountFlagBits samples);

    PipelineBuilder& setDepthState(bool testEnable, bool writeEnable, VkCompareOp compareOp);
    PipelineBuilder& setBlendEnable(bool enable);
    PipelineBuilder& setDynamicStates(const std::vector<VkDynamicState>& states);

    PipelineBuilder& setPipelineLayout(VkPipelineLayout layout);
    PipelineBuilder& setRenderPass(VkRenderPass renderPass);

    VkPipeline build(VkDevice device) const;

    static std::vector<char> readShaderFile(const std::string& filename);
    static VkShaderModule createShaderModule(VkDevice device, const std::string& filename);
    static VkPipeline createComputePipeline(VkDevice device, VkShaderModule computeShader, VkPipelineLayout layout);

private:
    VkPipelineShaderStageCreateInfo m_vertStage{};
    VkPipelineShaderStageCreateInfo m_fragStage{};
    bool m_hasVertStage = false;
    bool m_hasFragStage = false;

    std::vector<VkVertexInputBindingDescription> m_bindings;
    std::vector<VkVertexInputAttributeDescription> m_attributes;

    VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags m_cullMode = VK_CULL_MODE_NONE;
    VkFrontFace m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

    bool m_depthTestEnable = true;
    bool m_depthWriteEnable = true;
    VkCompareOp m_depthCompareOp = VK_COMPARE_OP_LESS;

    bool m_blendEnable = false;

    std::vector<VkDynamicState> m_dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};