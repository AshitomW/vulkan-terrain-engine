#include "core/PipelineBuilder.hpp"
#include <fstream>
#include <filesystem>
#include <array>

std::vector<char> PipelineBuilder::readShaderFile(const std::string& filename) {
    const std::vector<std::filesystem::path> searchPaths = {
        filename,
        std::filesystem::path("build") / filename,
        std::filesystem::path("..") / filename,
        std::filesystem::path("../build") / filename,
        std::filesystem::path("shaders") / filename,
        std::filesystem::path("build/shaders") / filename
    };

    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
            file.close();
            return buffer;
        }
    }

    throw std::runtime_error("Failed to open shader file: " + filename);
}

VkShaderModule PipelineBuilder::createShaderModule(VkDevice device, const std::string& filename) {
    std::vector<char> code = readShaderFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module for: " + filename);
    return shaderModule;
}

VkPipeline PipelineBuilder::createComputePipeline(VkDevice device, VkShaderModule computeShader, VkPipelineLayout layout) {
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.layout = layout;
    createInfo.stage = stageInfo;

    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline), "Failed to create compute pipeline");
    return pipeline;
}

PipelineBuilder& PipelineBuilder::setVertexShader(VkShaderModule module) {
    m_vertStage = {};
    m_vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    m_vertStage.module = module;
    m_vertStage.pName = "main";
    m_hasVertStage = true;
    return *this;
}

PipelineBuilder& PipelineBuilder::setFragmentShader(VkShaderModule module) {
    m_fragStage = {};
    m_fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_fragStage.module = module;
    m_fragStage.pName = "main";
    m_hasFragStage = true;
    return *this;
}

PipelineBuilder& PipelineBuilder::setVertexInput(
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes
) {
    m_bindings = bindings;
    m_attributes = attributes;
    return *this;
}

PipelineBuilder& PipelineBuilder::setTopology(VkPrimitiveTopology topology) {
    m_topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    m_polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode) {
    m_cullMode = cullMode;
    return *this;
}

PipelineBuilder& PipelineBuilder::setFrontFace(VkFrontFace frontFace) {
    m_frontFace = frontFace;
    return *this;
}

PipelineBuilder& PipelineBuilder::setRasterizationSamples(VkSampleCountFlagBits samples) {
    m_samples = samples;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthState(bool testEnable, bool writeEnable, VkCompareOp compareOp) {
    m_depthTestEnable = testEnable;
    m_depthWriteEnable = writeEnable;
    m_depthCompareOp = compareOp;
    return *this;
}

PipelineBuilder& PipelineBuilder::setBlendEnable(bool enable) {
    m_blendEnable = enable;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDynamicStates(const std::vector<VkDynamicState>& states) {
    m_dynamicStates = states;
    return *this;
}

PipelineBuilder& PipelineBuilder::setPipelineLayout(VkPipelineLayout layout) {
    m_pipelineLayout = layout;
    return *this;
}

PipelineBuilder& PipelineBuilder::setRenderPass(VkRenderPass renderPass) {
    m_renderPass = renderPass;
    return *this;
}

VkPipeline PipelineBuilder::build(VkDevice device) const {
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    uint32_t stageCount = 0;
    if (m_hasVertStage) shaderStages[stageCount++] = m_vertStage;
    if (m_hasFragStage) shaderStages[stageCount++] = m_fragStage;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindings.size());
    vertexInput.pVertexBindingDescriptions = m_bindings.empty() ? nullptr : m_bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributes.size());
    vertexInput.pVertexAttributeDescriptions = m_attributes.empty() ? nullptr : m_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = m_polygonMode;
    rasterizer.cullMode = m_cullMode;
    rasterizer.frontFace = m_frontFace;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = m_samples;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = m_depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = m_depthCompareOp;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = m_blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
    dynamicState.pDynamicStates = m_dynamicStates.empty() ? nullptr : m_dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = stageCount;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline");
    return pipeline;
}