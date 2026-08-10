#include "renderer/FoliageRenderer.hpp"
#include <cmath>
#include <iostream>
#include <random>

namespace {

uint32_t hashCPU(glm::uvec2 p, uint32_t seed) {
    uint32_t h = seed;
    h ^= p.x + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= p.y + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

glm::vec2 hash2CPU(glm::vec2 p, uint32_t seed) {
    glm::uvec2 ip = glm::uvec2(glm::ivec2(glm::floor(p)) + 100000);
    uint32_t h1 = hashCPU(ip, seed);
    uint32_t h2 = hashCPU(ip + glm::uvec2(137u, 269u), seed);
    return glm::vec2(
        static_cast<float>(h1 & 0x00FFFFFFu) / 16777215.0f,
        static_cast<float>(h2 & 0x00FFFFFFu) / 16777215.0f
    ) * 2.0f - 1.0f;
}

float gradientNoiseCPU(glm::vec2 p, uint32_t seed) {
    glm::vec2 i = glm::floor(p);
    glm::vec2 f = glm::fract(p);

    glm::vec2 u = f * f * f * (f * (f * 6.0f - 15.0f) + 10.0f);

    glm::vec2 g00 = hash2CPU(i + glm::vec2(0.0f, 0.0f), seed);
    glm::vec2 g10 = hash2CPU(i + glm::vec2(1.0f, 0.0f), seed);
    glm::vec2 g01 = hash2CPU(i + glm::vec2(0.0f, 1.0f), seed);
    glm::vec2 g11 = hash2CPU(i + glm::vec2(1.0f, 1.0f), seed);

    float n00 = glm::dot(g00, f - glm::vec2(0.0f, 0.0f));
    float n10 = glm::dot(g10, f - glm::vec2(1.0f, 0.0f));
    float n01 = glm::dot(g01, f - glm::vec2(0.0f, 1.0f));
    float n11 = glm::dot(g11, f - glm::vec2(1.0f, 1.0f));

    return glm::mix(
        glm::mix(n00, n10, u.x),
        glm::mix(n01, n11, u.x),
        u.y
    );
}

float fbmCPU(glm::vec2 p, uint32_t seed, int octaves, float lacunarity, float persistence) {
    float sum = 0.0f;
    float amp = 1.0f;
    float maxAmp = 0.0f;
    float freq = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        sum += amp * gradientNoiseCPU(p * freq, seed + static_cast<uint32_t>(i * 31));
        maxAmp += amp;
        freq *= lacunarity;
        amp *= persistence;
    }

    return sum / maxAmp;
}

float evaluatePresetHeightCPU(glm::vec2 worldPos, uint32_t presetType, float baseFreq, float amp, float warpStrength, float mountainPower, uint32_t seed) {
    if (presetType == 0u) {
        float mFreq = baseFreq * 2.2f;
        glm::vec2 p = worldPos * mFreq;

        glm::vec2 q = glm::vec2(
            fbmCPU(p * 0.5f, seed + 11u, 4, 2.0f, 0.5f),
            fbmCPU(p * 0.5f + glm::vec2(5.2f, 1.3f), seed + 22u, 4, 2.0f, 0.5f)
        );
        glm::vec2 warped = p + warpStrength * q;

        float chain = fbmCPU(warped * 0.35f, seed + 33u, 4, 2.0f, 0.5f);
        float chainMask = glm::smoothstep(-0.30f, 0.35f, chain);

        float r1 = 1.0f - std::abs(gradientNoiseCPU(warped * 0.75f, seed + 101u));
        float r2 = 1.0f - std::abs(gradientNoiseCPU(warped * 1.60f, seed + 102u));
        float r3 = 1.0f - std::abs(gradientNoiseCPU(warped * 3.30f, seed + 103u));
        float r4 = 1.0f - std::abs(gradientNoiseCPU(warped * 6.80f, seed + 104u));

        float peakStructure = (r1 * 0.50f + r2 * 0.30f + r3 * 0.14f + r4 * 0.06f);
        peakStructure = std::pow(std::max(0.0f, peakStructure), 2.1f) * 2.2f;

        float valley = std::abs(gradientNoiseCPU(warped * 0.45f + glm::vec2(2.3f, 7.1f), seed + 200u));
        float valleyCarve = glm::smoothstep(0.04f, 0.60f, valley);

        float lakeBasin = fbmCPU(warped * 0.25f, seed + 250u, 3, 2.0f, 0.5f);
        float lakeDip = glm::smoothstep(-0.4f, 0.2f, lakeBasin);

        float h = glm::mix(chain * 0.15f - 0.05f, peakStructure * mountainPower * valleyCarve, chainMask * lakeDip);
        h = h * 0.88f + 0.04f;
        return h * amp;

    } else if (presetType == 1u) {
        float hFreq = baseFreq * 4.2f;
        glm::vec2 p = worldPos * hFreq;

        glm::vec2 q = glm::vec2(
            gradientNoiseCPU(p * 0.35f, seed + 10u),
            gradientNoiseCPU(p * 0.35f + glm::vec2(4.1f, 2.7f), seed + 20u)
        );
        glm::vec2 warped = p + warpStrength * 0.35f * q;

        float hill1 = std::sin(warped.x * 0.65f) * std::cos(warped.y * 0.65f) * 0.25f;
        float hill2 = (gradientNoiseCPU(warped * 0.75f, seed + 30u) * 0.5f + 0.5f);
        float hill3 = (gradientNoiseCPU(warped * 1.50f, seed + 40u) * 0.5f + 0.5f) * 0.35f;
        float hill4 = (gradientNoiseCPU(warped * 3.00f, seed + 50u) * 0.5f + 0.5f) * 0.12f;
        float hill5 = gradientNoiseCPU(warped * 6.00f, seed + 60u) * 0.04f;

        float river = std::abs(gradientNoiseCPU(warped * 0.25f + glm::vec2(1.2f, 8.4f), seed + 70u));
        float riverCarve = glm::smoothstep(0.02f, 0.25f, river);

        float rollingMounds = std::pow(std::max(0.0f, hill2 + hill3 + hill4 + hill1), 1.30f) * 0.50f + hill5;
        float h = (rollingMounds * riverCarve) * 0.85f + 0.08f;
        return h * amp;

    } else if (presetType == 2u) {
        float cFreq = baseFreq * 2.8f;
        glm::vec2 p = worldPos * cFreq;

        glm::vec2 q = glm::vec2(
            fbmCPU(p * 0.55f, seed + 15u, 3, 2.0f, 0.5f),
            fbmCPU(p * 0.55f + glm::vec2(7.3f, 1.9f), seed + 25u, 3, 2.0f, 0.5f)
        );
        glm::vec2 warped = p + warpStrength * 0.9f * q;

        float baseMesa = fbmCPU(warped * 0.5f, seed + 50u, 4, 2.0f, 0.5f);
        float numSteps = 8.0f;
        float stepped = std::floor(baseMesa * numSteps) / numSteps;
        float frac = (baseMesa * numSteps) - std::floor(baseMesa * numSteps);
        float terracedMesa = stepped + glm::smoothstep(0.0f, 0.28f, frac) / numSteps;

        float chasm1 = std::abs(fbmCPU(warped * 0.70f, seed + 70u, 4, 2.0f, 0.5f));
        float chasm2 = std::abs(fbmCPU(warped * 1.35f + glm::vec2(3.1f, 5.7f), seed + 80u, 3, 2.0f, 0.5f));
        float gorge = std::min(chasm1, chasm2);
        float gorgeCarve = glm::smoothstep(0.04f, 0.35f, gorge);

        float buttes = 1.0f - std::abs(gradientNoiseCPU(warped * 1.7f, seed + 90u));
        buttes = std::pow(std::max(0.0f, buttes), 3.2f) * 0.30f;

        float h = (terracedMesa * gorgeCarve + buttes) * 0.85f + 0.06f;
        return h * amp;

    } else {
        float iFreq = baseFreq * 2.6f;
        glm::vec2 p = worldPos * iFreq;

        glm::vec2 q = glm::vec2(
            gradientNoiseCPU(p * 0.5f, seed + 12u),
            gradientNoiseCPU(p * 0.5f + glm::vec2(3.7f, 8.2f), seed + 24u)
        );
        glm::vec2 warped = p + warpStrength * 0.55f * q;

        float oceanBase = fbmCPU(warped * 0.45f, seed + 10u, 4, 2.0f, 0.5f);
        float islandPeak = std::pow(std::max(0.0f, 1.0f - std::abs(gradientNoiseCPU(warped * 1.25f, seed + 60u))), 2.0f);
        float volcanoCrater = std::abs(gradientNoiseCPU(warped * 2.5f, seed + 70u)) * 0.25f;

        float islandMask = glm::smoothstep(-0.06f, 0.35f, oceanBase);
        float h = glm::mix(-0.25f, (islandPeak * 1.3f - volcanoCrater) * 1.25f, islandMask);
        return h * amp;
    }
}

}

FoliageRenderer::FoliageRenderer(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout)
    : m_device(context.getDevice()) {
    createModels(context);
    createPipeline(context, renderPass, uboSetLayout);

    m_instancesByType.resize(NUM_MODELS);
    m_instanceBuffers.resize(NUM_MODELS);
    m_instanceCounts.resize(NUM_MODELS, 0);
}

FoliageRenderer::~FoliageRenderer() {
    for (auto& buf : m_instanceBuffers) {
        buf.destroy();
    }
    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

void FoliageRenderer::createModels(const VulkanContext& context) {
    std::vector<FoliageVertex> vertices;
    std::vector<uint32_t> indices;
    m_modelMeshes.resize(NUM_MODELS);

    auto addCylinder = [&](float radius, float minY, float maxY, int sides, glm::vec3 col) {
        uint32_t base = static_cast<uint32_t>(vertices.size());
        for (int i = 0; i < sides; ++i) {
            float a = static_cast<float>(i) / static_cast<float>(sides) * 2.0f * 3.14159f;
            float x = std::cos(a) * radius;
            float z = std::sin(a) * radius;
            glm::vec3 n(std::cos(a), 0.0f, std::sin(a));
            vertices.push_back({glm::vec3(x, minY, z), n, col});
            vertices.push_back({glm::vec3(x, maxY, z), n, col});
        }
        for (int i = 0; i < sides; ++i) {
            int next = (i + 1) % sides;
            indices.push_back(base + i * 2); indices.push_back(base + i * 2 + 1); indices.push_back(base + next * 2);
            indices.push_back(base + next * 2); indices.push_back(base + i * 2 + 1); indices.push_back(base + next * 2 + 1);
        }
    };

    auto addCone = [&](float baseY, float topY, float radius, int sides, glm::vec3 col) {
        uint32_t tipIdx = static_cast<uint32_t>(vertices.size());
        vertices.push_back({glm::vec3(0.0f, topY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), col});
        uint32_t base = static_cast<uint32_t>(vertices.size());

        for (int i = 0; i < sides; ++i) {
            float a = static_cast<float>(i) / static_cast<float>(sides) * 2.0f * 3.14159f;
            float x = std::cos(a) * radius;
            float z = std::sin(a) * radius;
            glm::vec3 n = glm::normalize(glm::vec3(std::cos(a), 0.4f, std::sin(a)));
            vertices.push_back({glm::vec3(x, baseY, z), n, col});
        }
        for (int i = 0; i < sides; ++i) {
            int next = (i + 1) % sides;
            indices.push_back(tipIdx);
            indices.push_back(base + i);
            indices.push_back(base + next);
        }
    };

    m_modelMeshes[0].firstIndex = static_cast<uint32_t>(indices.size());
    addCylinder(0.38f, -1.2f, 3.2f, 6, glm::vec3(0.18f, 0.12f, 0.08f));
    addCone(1.8f, 5.5f, 2.4f, 7, glm::vec3(0.03f, 0.12f, 0.04f));
    addCone(4.2f, 8.0f, 1.8f, 7, glm::vec3(0.04f, 0.14f, 0.05f));
    addCone(6.5f, 10.5f, 1.2f, 7, glm::vec3(0.05f, 0.16f, 0.06f));
    m_modelMeshes[0].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[0].firstIndex;

    m_modelMeshes[1].firstIndex = static_cast<uint32_t>(indices.size());
    addCylinder(0.48f, -1.2f, 4.0f, 6, glm::vec3(0.20f, 0.14f, 0.09f));
    addCone(3.2f, 7.5f, 2.8f, 8, glm::vec3(0.08f, 0.22f, 0.06f));
    addCone(5.0f, 8.8f, 2.2f, 8, glm::vec3(0.10f, 0.25f, 0.07f));
    m_modelMeshes[1].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[1].firstIndex;

    m_modelMeshes[2].firstIndex = static_cast<uint32_t>(indices.size());
    addCylinder(0.32f, -1.2f, 6.2f, 6, glm::vec3(0.32f, 0.24f, 0.16f));
    for (int p = 0; p < 6; ++p) {
        float ang = static_cast<float>(p) / 6.0f * 2.0f * 3.14159f;
        float dx = std::cos(ang) * 3.2f;
        float dz = std::sin(ang) * 3.2f;
        uint32_t pBase = static_cast<uint32_t>(vertices.size());
        glm::vec3 frondCol(0.08f, 0.26f, 0.08f);
        vertices.push_back({glm::vec3(0.0f, 6.2f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), frondCol});
        vertices.push_back({glm::vec3(dx * 0.5f, 6.6f, dz * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), frondCol});
        vertices.push_back({glm::vec3(dx, 4.8f, dz), glm::vec3(0.0f, 1.0f, 0.0f), frondCol});
        indices.push_back(pBase); indices.push_back(pBase + 1); indices.push_back(pBase + 2);
    }
    m_modelMeshes[2].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[2].firstIndex;

    m_modelMeshes[3].firstIndex = static_cast<uint32_t>(indices.size());
    glm::vec3 cactusCol(0.14f, 0.26f, 0.12f);
    addCylinder(0.42f, -1.0f, 5.2f, 6, cactusCol);
    addCylinder(0.26f, 2.0f, 4.2f, 5, cactusCol);
    m_modelMeshes[3].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[3].firstIndex;

    m_modelMeshes[4].firstIndex = static_cast<uint32_t>(indices.size());
    uint32_t bBase = static_cast<uint32_t>(vertices.size());
    glm::vec3 rockCol(0.14f, 0.15f, 0.18f);
    vertices.push_back({glm::vec3( 0.0f,  1.4f,  0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), rockCol * 1.15f});
    vertices.push_back({glm::vec3(-1.2f,  0.3f, -0.9f), glm::vec3(-0.7f, 0.5f, -0.5f), rockCol});
    vertices.push_back({glm::vec3( 1.1f,  0.4f, -1.0f), glm::vec3( 0.7f, 0.5f, -0.5f), rockCol * 0.9f});
    vertices.push_back({glm::vec3( 1.3f,  0.2f,  0.9f), glm::vec3( 0.7f, 0.5f,  0.5f), rockCol});
    vertices.push_back({glm::vec3(-1.0f,  0.5f,  1.1f), glm::vec3(-0.6f, 0.5f,  0.6f), rockCol * 1.05f});
    vertices.push_back({glm::vec3( 0.0f, -0.8f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), rockCol * 0.7f});

    indices.push_back(bBase + 0); indices.push_back(bBase + 1); indices.push_back(bBase + 2);
    indices.push_back(bBase + 0); indices.push_back(bBase + 2); indices.push_back(bBase + 3);
    indices.push_back(bBase + 0); indices.push_back(bBase + 3); indices.push_back(bBase + 4);
    indices.push_back(bBase + 0); indices.push_back(bBase + 4); indices.push_back(bBase + 1);
    indices.push_back(bBase + 5); indices.push_back(bBase + 2); indices.push_back(bBase + 1);
    indices.push_back(bBase + 5); indices.push_back(bBase + 3); indices.push_back(bBase + 2);
    indices.push_back(bBase + 5); indices.push_back(bBase + 4); indices.push_back(bBase + 3);
    indices.push_back(bBase + 5); indices.push_back(bBase + 1); indices.push_back(bBase + 4);
    m_modelMeshes[4].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[4].firstIndex;

    m_modelMeshes[5].firstIndex = static_cast<uint32_t>(indices.size());
    uint32_t cBase = static_cast<uint32_t>(vertices.size());
    glm::vec3 sandstoneCol(0.48f, 0.20f, 0.10f);
    vertices.push_back({glm::vec3( 0.0f,  1.2f,  0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), sandstoneCol * 1.15f});
    vertices.push_back({glm::vec3(-1.0f,  0.3f, -0.8f), glm::vec3(-0.7f, 0.5f, -0.5f), sandstoneCol});
    vertices.push_back({glm::vec3( 1.0f,  0.4f, -0.8f), glm::vec3( 0.7f, 0.5f, -0.5f), sandstoneCol * 0.9f});
    vertices.push_back({glm::vec3( 1.1f,  0.2f,  0.8f), glm::vec3( 0.7f, 0.5f,  0.5f), sandstoneCol});
    vertices.push_back({glm::vec3(-0.9f,  0.5f,  0.9f), glm::vec3(-0.6f, 0.5f,  0.6f), sandstoneCol * 1.05f});
    vertices.push_back({glm::vec3( 0.0f, -0.8f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), sandstoneCol * 0.7f});

    indices.push_back(cBase + 0); indices.push_back(cBase + 1); indices.push_back(cBase + 2);
    indices.push_back(cBase + 0); indices.push_back(cBase + 2); indices.push_back(cBase + 3);
    indices.push_back(cBase + 0); indices.push_back(cBase + 3); indices.push_back(cBase + 4);
    indices.push_back(cBase + 0); indices.push_back(cBase + 4); indices.push_back(cBase + 1);
    indices.push_back(cBase + 5); indices.push_back(cBase + 2); indices.push_back(cBase + 1);
    indices.push_back(cBase + 5); indices.push_back(cBase + 3); indices.push_back(cBase + 2);
    indices.push_back(cBase + 5); indices.push_back(cBase + 4); indices.push_back(cBase + 3);
    indices.push_back(cBase + 5); indices.push_back(cBase + 1); indices.push_back(cBase + 4);
    m_modelMeshes[5].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[5].firstIndex;

    m_modelMeshes[6].firstIndex = static_cast<uint32_t>(indices.size());
    glm::vec3 gColBottom(0.06f, 0.16f, 0.04f);
    glm::vec3 gColTop(0.12f, 0.28f, 0.08f);

    float gWidth = 1.0f;
    float gHeight = 1.4f;
    for (int q = 0; q < 3; ++q) {
        float angle = static_cast<float>(q) * (3.14159f / 3.0f);
        float dx = std::cos(angle) * gWidth;
        float dz = std::sin(angle) * gWidth;
        uint32_t qBase = static_cast<uint32_t>(vertices.size());
        glm::vec3 n(std::sin(angle), 0.2f, -std::cos(angle));

        vertices.push_back({glm::vec3(-dx, -0.5f, -dz), n, gColBottom});
        vertices.push_back({glm::vec3( dx, -0.5f,  dz), n, gColBottom});
        vertices.push_back({glm::vec3(-dx * 0.8f, gHeight, -dz * 0.8f), n, gColTop});
        vertices.push_back({glm::vec3( dx * 0.8f, gHeight,  dz * 0.8f), n, gColTop});

        indices.push_back(qBase + 0); indices.push_back(qBase + 2); indices.push_back(qBase + 1);
        indices.push_back(qBase + 1); indices.push_back(qBase + 2); indices.push_back(qBase + 3);
    }
    m_modelMeshes[6].indexCount = static_cast<uint32_t>(indices.size()) - m_modelMeshes[6].firstIndex;
    m_totalIndexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vSize = sizeof(FoliageVertex) * vertices.size();
    VulkanBuffer vStaging(context, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vStaging.copyFromHost(vertices.data(), vSize);
    m_vertexBuffer.create(context, vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanBuffer::copyBuffer(context, vStaging.getBuffer(), m_vertexBuffer.getBuffer(), vSize);

    VkDeviceSize iSize = sizeof(uint32_t) * indices.size();
    VulkanBuffer iStaging(context, iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    iStaging.copyFromHost(indices.data(), iSize);
    m_indexBuffer.create(context, iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanBuffer::copyBuffer(context, iStaging.getBuffer(), m_indexBuffer.getBuffer(), iSize);
}

void FoliageRenderer::createPipeline(const VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout uboSetLayout) {
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &uboSetLayout;

    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout), "Failed to create foliage pipeline layout");

    VkShaderModule vertShader = VulkanPipeline::createShaderModule(m_device, "shaders/foliage.vert.spv");
    VkShaderModule fragShader = VulkanPipeline::createShaderModule(m_device, "shaders/foliage.frag.spv");

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShader;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShader;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    std::vector<VkVertexInputBindingDescription> bindings = {
        {0, sizeof(FoliageVertex), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(FoliageInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
    };

    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FoliageVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FoliageVertex, normal)},
        {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FoliageVertex, color)},
        {3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(FoliageInstance, posScale)},
        {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(FoliageInstance, params)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create foliage graphics pipeline");

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

float FoliageRenderer::sampleExactHeight(glm::vec2 worldPos, const TerrainConfig& config) {
    float baseFreq = config.frequency * 0.004f;
    float amp = config.amplitude;
    float warpStrength = config.warpStrength;
    float mountainPower = config.mountainPower;
    uint32_t seed = config.seed;
    uint32_t presetType = config.presetType;

    if (presetType < 4u) {
        return evaluatePresetHeightCPU(worldPos, presetType, baseFreq, amp, warpStrength, mountainPower, seed);
    }

    glm::vec2 macroP = worldPos * 0.00045f;
    float continent = fbmCPU(macroP, seed + 500u, 4, 2.0f, 0.5f);
    float moisture  = fbmCPU(macroP + glm::vec2(12.3f, 45.6f), seed + 600u, 4, 2.0f, 0.5f);

    float wOcean = glm::smoothstep(0.0f, -0.25f, continent);
    float wHighland = glm::smoothstep(-0.05f, 0.25f, continent);
    float wHills = 1.0f - std::max(wOcean, wHighland);
    float wCanyon = wHighland * glm::smoothstep(-0.1f, 0.2f, moisture);
    float wMountain = wHighland * (1.0f - glm::smoothstep(-0.1f, 0.2f, moisture));

    float hMtn = evaluatePresetHeightCPU(worldPos, 0u, baseFreq, amp * 1.10f, warpStrength, mountainPower, seed);
    float hHill = evaluatePresetHeightCPU(worldPos, 1u, baseFreq, amp * 0.40f, warpStrength * 0.5f, mountainPower * 0.6f, seed);
    float hCanyon = evaluatePresetHeightCPU(worldPos, 2u, baseFreq, amp * 0.85f, warpStrength, mountainPower, seed);
    float hIsland = evaluatePresetHeightCPU(worldPos, 3u, baseFreq, amp * 0.65f, warpStrength, mountainPower, seed);

    float totalWeight = wOcean + wHills + wMountain + wCanyon + 0.0001f;
    return (hIsland * wOcean + hHill * wHills + hMtn * wMountain + hCanyon * wCanyon) / totalWeight;
}

glm::vec3 FoliageRenderer::sampleExactNormal(glm::vec2 worldPos, const TerrainConfig& config, float  ) {
    float eps = CHUNK_CELL_SIZE * 0.5f;
    float hL = sampleExactHeight(worldPos - glm::vec2(eps, 0.0f), config);
    float hR = sampleExactHeight(worldPos + glm::vec2(eps, 0.0f), config);
    float hD = sampleExactHeight(worldPos - glm::vec2(0.0f, eps), config);
    float hU = sampleExactHeight(worldPos + glm::vec2(0.0f, eps), config);

    return glm::normalize(glm::vec3(
        (hL - hR) / (2.0f * eps),
        1.0f,
        (hD - hU) / (2.0f * eps)
    ));
}

void FoliageRenderer::generateScatterForChunk(
    glm::vec2 chunkOrigin,
    const TerrainConfig& config,
    std::vector<FoliageInstance>& outInstances
) {
    if (!config.showFoliage || config.foliageDensity <= 0.001f) return;

    uint32_t chunkSeed = static_cast<uint32_t>(std::abs(chunkOrigin.x * 73856093.0f + chunkOrigin.y * 19349663.0f)) + config.seed;
    std::mt19937 gen(chunkSeed);
    std::uniform_real_distribution<float> distOffset(3.0f, CHUNK_SIZE - 3.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 6.28318f);
    std::uniform_real_distribution<float> distScale(0.85f, 1.25f);
    std::uniform_real_distribution<float> distVar(-0.25f, 0.25f);
    std::uniform_real_distribution<float> distProb(0.0f, 1.0f);

    int numCandidates = static_cast<int>(50.0f * config.foliageDensity);

    for (int i = 0; i < numCandidates; ++i) {
        float localX = distOffset(gen);
        float localZ = distOffset(gen);
        glm::vec2 worldXZ = chunkOrigin + glm::vec2(localX, localZ);

        float height = sampleExactHeight(worldXZ, config);

        if (height < config.waterHeight + 0.5f) {
            continue;
        }

        glm::vec3 normal = sampleExactNormal(worldXZ, config, height);
        float slope = 1.0f - glm::clamp(normal.y, 0.0f, 1.0f);

        float relH = (height - config.waterHeight) / config.amplitude;

        uint32_t effectiveBiome = config.presetType;
        if (effectiveBiome == 4u) {
            glm::vec2 macroP = worldXZ * 0.00045f;
            float continent = fbmCPU(macroP, config.seed + 500u, 4, 2.0f, 0.5f);
            float moisture  = fbmCPU(macroP + glm::vec2(12.3f, 45.6f), config.seed + 600u, 4, 2.0f, 0.5f);

            if (continent < -0.15f) {
                effectiveBiome = 3u;
            } else if (continent > 0.15f && moisture < 0.0f) {
                effectiveBiome = 0u;
            } else if (continent > 0.15f && moisture >= 0.0f) {
                effectiveBiome = 2u;
            } else {
                effectiveBiome = 1u;
            }
        }

        uint32_t objType = 0;
        float prob = distProb(gen);
        float scale = distScale(gen);

        if (effectiveBiome == 0u) {

            if (relH > 0.70f) {
                objType = 4;
                if (prob < 0.60f) continue;
            } else if (slope > 0.45f) {
                objType = 4;
                if (prob < 0.50f) continue;
            } else if (relH < 0.25f && prob < 0.35f) {
                objType = 6;
                scale *= 1.3f;
            } else {
                objType = 0;
                if (prob < 0.25f) objType = 4;
            }

        } else if (effectiveBiome == 1u) {

            if (slope > 0.50f) {
                objType = 4;
                if (prob < 0.60f) continue;
            } else if (prob < 0.40f) {
                objType = 1;
            } else if (prob < 0.85f) {
                objType = 6;
                scale *= 1.35f;
            } else {
                objType = 4;
                scale *= 0.75f;
            }

        } else if (effectiveBiome == 2u) {

            if (slope > 0.40f) {
                objType = 5;
                if (prob < 0.50f) continue;
            } else if (prob < 0.45f) {
                objType = 3;
                scale *= 1.15f;
            } else if (prob < 0.80f) {
                objType = 5;
            } else {
                continue;
            }

        } else {

            if (slope > 0.50f) {
                objType = 4;
                if (prob < 0.60f) continue;
            } else if (relH < 0.15f || prob < 0.55f) {
                objType = 2;
                scale *= 1.25f;
            } else if (prob < 0.80f) {
                objType = 6;
                scale *= 1.4f;
            } else {
                objType = 4;
                scale *= 0.85f;
            }
        }

        FoliageInstance inst{};

        inst.posScale = glm::vec4(worldXZ.x, height - 0.15f, worldXZ.y, scale);
        inst.params = glm::vec4(distRot(gen), static_cast<float>(objType), distVar(gen), 0.0f);
        outInstances.push_back(inst);
    }
}

void FoliageRenderer::updateInstances(
    const VulkanContext& context,
    const std::vector<glm::vec2>& chunkOrigins,
    const TerrainConfig& config
) {
    for (size_t i = 0; i < NUM_MODELS; ++i) {
        m_instancesByType[i].clear();
    }

    if (!config.showFoliage || config.foliageDensity <= 0.001f) {
        for (size_t i = 0; i < NUM_MODELS; ++i) {
            m_instanceCounts[i] = 0;
        }
        return;
    }

    std::vector<FoliageInstance> allInstances;
    allInstances.reserve(chunkOrigins.size() * static_cast<size_t>(50.0f * config.foliageDensity));

    for (const auto& origin : chunkOrigins) {
        generateScatterForChunk(origin, config, allInstances);
    }

    for (const auto& inst : allInstances) {
        uint32_t type = static_cast<uint32_t>(inst.params.y);
        if (type < NUM_MODELS) {
            m_instancesByType[type].push_back(inst);
        }
    }

    for (uint32_t type = 0; type < NUM_MODELS; ++type) {
        m_instanceCounts[type] = static_cast<uint32_t>(m_instancesByType[type].size());
        if (m_instanceCounts[type] == 0) continue;

        VkDeviceSize bufferSize = sizeof(FoliageInstance) * m_instanceCounts[type];

        VulkanBuffer stagingBuffer(
            context,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        stagingBuffer.copyFromHost(m_instancesByType[type].data(), bufferSize);

        m_instanceBuffers[type].create(
            context,
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VulkanBuffer::copyBuffer(context, stagingBuffer.getBuffer(), m_instanceBuffers[type].getBuffer(), bufferSize);
    }
}

void FoliageRenderer::recordRenderCommands(
    VkCommandBuffer commandBuffer,
    VkPipelineLayout  ,
    VkDescriptorSet uboDescriptorSet
) {
    bool hasAny = false;
    for (uint32_t c : m_instanceCounts) {
        if (c > 0) { hasAny = true; break; }
    }
    if (!hasAny) return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0,
        1,
        &uboDescriptorSet,
        0,
        nullptr
    );

    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    for (uint32_t type = 0; type < NUM_MODELS; ++type) {
        if (m_instanceCounts[type] == 0) continue;

        VkBuffer vBuffers[] = {m_vertexBuffer.getBuffer(), m_instanceBuffers[type].getBuffer()};
        VkDeviceSize offsets[] = {0, 0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vBuffers, offsets);

        const auto& mesh = m_modelMeshes[type];
        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, m_instanceCounts[type], mesh.firstIndex, 0, 0);
    }
}
