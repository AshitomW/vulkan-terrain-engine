#pragma once

#include "core/VulkanContext.hpp"
#include <vector>
#include <string>

class VulkanPipeline {
public:
    static VkShaderModule createShaderModule(VkDevice device, const std::string& filename);
    static std::vector<char> readShaderFile(const std::string& filename);

    static void createComputePipeline(
        VkDevice device,
        VkShaderModule computeShader,
        VkPipelineLayout layout,
        VkPipeline& outPipeline
    );

    static void createTerrainGraphicsPipeline(
        VkDevice device,
        VkRenderPass renderPass,
        VkPipelineLayout layout,
        VkShaderModule vertShader,
        VkShaderModule fragShader,
        bool wireframe,
        VkPipeline& outPipeline
    );
};
