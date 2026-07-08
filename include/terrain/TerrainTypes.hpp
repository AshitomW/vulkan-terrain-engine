#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <cstdint>
#include <algorithm>
#include <cmath>

static constexpr uint32_t CHUNK_GRID_RES = 65;
static constexpr float CHUNK_SIZE = 128.0f;
static constexpr float CHUNK_CELL_SIZE = CHUNK_SIZE / static_cast<float>(CHUNK_GRID_RES - 1);
static constexpr uint32_t NUM_LOD_LEVELS = 4;

struct TerrainConfig {
    uint32_t seed = 1337;
    float frequency = 1.0f;
    float amplitude = 110.0f;
    float warpStrength = 0.85f;
    float mountainPower = 1.8f;
    float octaves = 6.0f;
    float lacunarity = 2.0f;
    float persistence = 0.45f;
    float waterHeight = -10.0f;
    float fogDensity = 0.00065f;
    float debugMode = 0.0f;
    uint32_t presetType = 0;
    int lodMode = -1;
    int viewRadius = 3;
    bool wireframe = false;
    bool showFoliage = true;
    float foliageDensity = 1.0f;

    float timeOfDay = 14.5f;
    bool timeCycleRunning = true;
    float timeCycleSpeed = 0.20f;

    bool isDynamicLOD() const { return lodMode == -1; }

    float getViewDistance() const {
        return static_cast<float>(2 * viewRadius + 1) * CHUNK_SIZE * 0.5f;
    }

    void increaseViewDistance() {
        viewRadius = std::min(8, viewRadius + 1);
        updateFog();
    }

    void decreaseViewDistance() {
        viewRadius = std::max(1, viewRadius - 1);
        updateFog();
    }

    void increaseWaterHeight() {
        waterHeight += 1.0f;
    }

    void decreaseWaterHeight() {
        waterHeight -= 1.0f;
    }

    void toggleFoliage() {
        showFoliage = !showFoliage;
    }

    void increaseFoliageDensity() {
        foliageDensity = std::min(3.0f, foliageDensity + 0.25f);
    }

    void decreaseFoliageDensity() {
        foliageDensity = std::max(0.0f, foliageDensity - 0.25f);
    }

    void advanceTime(float deltaSeconds) {
        if (timeCycleRunning) {
            timeOfDay = std::fmod(timeOfDay + deltaSeconds * timeCycleSpeed, 24.0f);
            if (timeOfDay < 0.0f) timeOfDay += 24.0f;
        }
    }

    void scrubTime(float deltaHours) {
        timeOfDay = std::fmod(timeOfDay + deltaHours, 24.0f);
        if (timeOfDay < 0.0f) timeOfDay += 24.0f;
    }

    void toggleTimeCycle() {
        timeCycleRunning = !timeCycleRunning;
    }

    void updateFog() {
        float visibleDist = static_cast<float>(2 * viewRadius + 1) * CHUNK_SIZE * 0.85f;
        fogDensity = 1.0f / std::max(100.0f, visibleDist);
    }

    void cycleLODMode() {
        lodMode++;
        if (lodMode >= static_cast<int>(NUM_LOD_LEVELS)) {
            lodMode = -1;
        }
    }

    void decreaseLOD() {
        if (lodMode == -1) {
            lodMode = 0;
        } else {
            lodMode = std::max(0, lodMode - 1);
        }
    }

    void increaseLOD() {
        if (lodMode == -1) {
            lodMode = 1;
        } else {
            lodMode = std::min(static_cast<int>(NUM_LOD_LEVELS) - 1, lodMode + 1);
        }
    }

    void applyMountains() {
        presetType = 0;
        amplitude = 115.0f;
        frequency = 1.0f;
        warpStrength = 0.85f;
        mountainPower = 1.8f;
        waterHeight = -10.0f;
        updateFog();
    }

    void applyHills() {
        presetType = 1;
        amplitude = 38.0f;
        frequency = 1.0f;
        warpStrength = 0.40f;
        mountainPower = 0.50f;
        waterHeight = -10.0f;
        updateFog();
    }

    void applyCanyons() {
        presetType = 2;
        amplitude = 80.0f;
        frequency = 1.0f;
        warpStrength = 1.00f;
        mountainPower = 1.50f;
        waterHeight = -10.0f;
        updateFog();
    }

    void applyIslands() {
        presetType = 3;
        amplitude = 60.0f;
        frequency = 1.0f;
        warpStrength = 0.65f;
        mountainPower = 1.30f;
        waterHeight = 8.0f;
        updateFog();
    }

    void applyMultiBiome() {
        presetType = 4;
        amplitude = 100.0f;
        frequency = 1.0f;
        warpStrength = 0.80f;
        mountainPower = 1.60f;
        waterHeight = 0.0f;
        updateFog();
    }
};

struct GlobalUBO {
    glm::mat4 viewProj;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos;
    glm::vec4 sunDir;
    glm::vec4 sunColor;
    glm::vec4 skyColorZenith;
    glm::vec4 skyColorHorizon;
    glm::vec4 terrainParams;
    glm::vec4 biomeParams;
};

struct ChunkPushConstants {
    glm::vec4 chunkOffset;
    glm::uvec4 lodParams;
};

struct ComputePushConstants {
    glm::vec4 chunkOffset;
    glm::vec4 noiseParams1;
    glm::vec4 noiseParams2;
    glm::uvec4 config;
};
