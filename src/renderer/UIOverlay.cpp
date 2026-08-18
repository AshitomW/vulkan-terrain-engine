#include "renderer/UIOverlay.hpp"
#include <cstring>

static const uint8_t font8x8_basic[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00},
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
    {0x00,0x66,0x6C,0x18,0x30,0x66,0x46,0x00},
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00},
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00},
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    {0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00},
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00},
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00},
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
    {0x3C,0x66,0x6E,0x6E,0x60,0x66,0x3C,0x00},
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3A,0x00},
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    {0x0E,0x06,0x06,0x06,0x06,0x66,0x3C,0x00},
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00},
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    {0x80,0xC0,0x60,0x30,0x18,0x0C,0x06,0x00},
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
    {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00},
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
    {0x06,0x00,0x06,0x06,0x06,0x06,0x66,0x3C},
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00},
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00},
    {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00},
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C},
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}
};

UIOverlay::UIOverlay(const VulkanContext& context, VkRenderPass renderPass)
    : m_device(context.getDevice()) {
    createFontTexture(context);
    createDescriptorResources(context);
    createPipeline(context, renderPass);
    createVertexBuffers(context);
    m_vertices.reserve(MAX_VERTICES);
}

void UIOverlay::createFontTexture(const VulkanContext& context) {

    uint32_t texWidth = 128;
    uint32_t texHeight = 64;
    std::vector<uint8_t> pixels(texWidth * texHeight, 0);

    for (int charIdx = 0; charIdx < 96; ++charIdx) {
        int charX = (charIdx % 16) * 8;
        int charY = (charIdx / 16) * 8;

        for (int row = 0; row < 8; ++row) {
            uint8_t rowBits = font8x8_basic[charIdx][row];
            for (int col = 0; col < 8; ++col) {
                if (rowBits & (1 << (7 - col))) {
                    pixels[(charY + row) * texWidth + (charX + col)] = 255;
                }
            }
        }
    }

    VkDeviceSize imageSize = texWidth * texHeight;

    VulkanBuffer stagingBuffer(
        context,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    stagingBuffer.copyFromHost(pixels.data(), imageSize);

    m_fontImage.create(
        context,
        texWidth,
        texHeight,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VkCommandBuffer cmd = context.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_fontImage.getImage();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {texWidth, texHeight, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer.getBuffer(), m_fontImage.getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    context.executeSingleTimeCommands(cmd);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    VkSampler sampler;
    VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler), "Failed to create font sampler");
    m_fontSampler = vkh::SamplerHandle(
        sampler,
        [device = m_device](VkSampler s) { vkDestroySampler(device, s, nullptr); }
    );
}

void UIOverlay::createDescriptorResources(const VulkanContext&) {
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &setLayout), "Failed to create UI set layout");
    m_descriptorSetLayout = vkh::DescriptorSetLayoutHandle(
        setLayout,
        [device = m_device](VkDescriptorSetLayout l) { vkDestroyDescriptorSetLayout(device, l, nullptr); }
    );

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &pool), "Failed to create UI descriptor pool");
    m_descriptorPool = vkh::DescriptorPoolHandle(
        pool,
        [device = m_device](VkDescriptorPool p) { vkDestroyDescriptorPool(device, p, nullptr); }
    );

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &setLayout;

    VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet), "Failed to allocate UI descriptor set");

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_fontImage.getView();
    imageInfo.sampler = m_fontSampler.get();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}

void UIOverlay::createPipeline(const VulkanContext&, VkRenderPass renderPass) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::vec2);

    VkDescriptorSetLayout setLayout = m_descriptorSetLayout.get();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &setLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &layout), "Failed to create UI pipeline layout");
    m_pipelineLayout = vkh::PipelineLayoutHandle(
        layout,
        [device = m_device](VkPipelineLayout l) { vkDestroyPipelineLayout(device, l, nullptr); }
    );

    VkShaderModule vertShader = PipelineBuilder::createShaderModule(m_device, "shaders/ui.vert.spv");
    VkShaderModule fragShader = PipelineBuilder::createShaderModule(m_device, "shaders/ui.frag.spv");

    std::vector<VkVertexInputBindingDescription> bindings = {
        {0, sizeof(UIVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIVertex, uv)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIVertex, color)}
    };

    PipelineBuilder builder;
    builder.setVertexShader(vertShader)
        .setFragmentShader(fragShader)
        .setVertexInput(bindings, attributes)
        .setCullMode(VK_CULL_MODE_NONE)
        .setDepthState(false, false, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setBlendEnable(true)
        .setPipelineLayout(layout)
        .setRenderPass(renderPass);

    m_pipeline = vkh::PipelineHandle(
        builder.build(m_device),
        [device = m_device](VkPipeline p) { vkDestroyPipeline(device, p, nullptr); }
    );

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void UIOverlay::createVertexBuffers(const VulkanContext& context) {
    VkDeviceSize bufferSize = sizeof(UIVertex) * MAX_VERTICES;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_vertexBuffers[i].create(
            context,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_vertexMapped[i] = m_vertexBuffers[i].map();
    }
}

void UIOverlay::begin() {
    m_vertices.clear();
}

void UIOverlay::drawRect(float x, float y, float w, float h, glm::vec4 color) {
    if (m_vertices.size() + 6 > MAX_VERTICES) return;

    glm::vec2 p0(x, y);
    glm::vec2 p1(x + w, y);
    glm::vec2 p2(x, y + h);
    glm::vec2 p3(x + w, y + h);
    glm::vec2 uv(-1.0f, -1.0f);

    m_vertices.push_back({p0, uv, color});
    m_vertices.push_back({p2, uv, color});
    m_vertices.push_back({p1, uv, color});

    m_vertices.push_back({p1, uv, color});
    m_vertices.push_back({p2, uv, color});
    m_vertices.push_back({p3, uv, color});
}

void UIOverlay::drawText(float x, float y, float scale, std::string_view text, glm::vec4 color) {
    float curX = x;
    float curY = y;
    float charW = 8.0f * scale;
    float charH = 8.0f * scale;

    for (char c : text) {
        if (c == '\n') {
            curX = x;
            curY += charH + 4.0f * scale;
            continue;
        }

        uint8_t uchar = static_cast<uint8_t>(c);
        if (uchar < 32 || uchar > 127) {
            uchar = '?';
        }
        int charIdx = uchar - 32;

        float u0 = float(charIdx % 16) * 8.0f / 128.0f;
        float v0 = float(charIdx / 16) * 8.0f / 64.0f;
        float u1 = u0 + 8.0f / 128.0f;
        float v1 = v0 + 8.0f / 64.0f;

        if (m_vertices.size() + 6 <= MAX_VERTICES) {
            glm::vec2 p0(curX, curY);
            glm::vec2 p1(curX + charW, curY);
            glm::vec2 p2(curX, curY + charH);
            glm::vec2 p3(curX + charW, curY + charH);

            m_vertices.push_back({p0, glm::vec2(u0, v0), color});
            m_vertices.push_back({p2, glm::vec2(u0, v1), color});
            m_vertices.push_back({p1, glm::vec2(u1, v0), color});

            m_vertices.push_back({p1, glm::vec2(u1, v0), color});
            m_vertices.push_back({p2, glm::vec2(u0, v1), color});
            m_vertices.push_back({p3, glm::vec2(u1, v1), color});
        }

        curX += charW;
    }
}

void UIOverlay::end(uint32_t currentFrame) {
    m_vertexCounts[currentFrame] = static_cast<uint32_t>(m_vertices.size());
    if (!m_vertices.empty() && m_vertexMapped[currentFrame]) {
        std::memcpy(
            m_vertexMapped[currentFrame],
            m_vertices.data(),
            sizeof(UIVertex) * m_vertices.size()
        );
    }
}

void UIOverlay::recordCommands(VkCommandBuffer cmd, uint32_t currentFrame, float screenWidth, float screenHeight) {
    uint32_t count = m_vertexCounts[currentFrame];
    if (count == 0) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());

    glm::vec2 screenSize(screenWidth, screenHeight);
    vkCmdPushConstants(cmd, m_pipelineLayout.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec2), &screenSize);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.get(), 0, 1, &m_descriptorSet, 0, nullptr);

    VkBuffer vertexBuffers[] = {m_vertexBuffers[currentFrame].getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(cmd, count, 1, 0, 0);
}
