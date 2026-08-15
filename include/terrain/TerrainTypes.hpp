#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "core/EngineConstants.hpp"

static constexpr uint32_t CHUNK_GRID_RES = EngineConstants::Terrain::GRID_RESOLUTION;
static constexpr float CHUNK_SIZE = EngineConstants::Terrain::CHUNK_SIZE;
static constexpr float CHUNK_CELL_SIZE = EngineConstants::Terrain::CELL_SIZE;
static constexpr uint32_t NUM_LOD_LEVELS = EngineConstants::Terrain::NUM_LOD_LEVELS;

struct TerrainConfig {
    uint32_t seed = EngineConstants::Terrain::DEFAULT_SEED;
    float frequency = 1.0f;
    float amplitude = EngineConstants::Presets::MOUNTAINS.amplitude;
    float warpStrength = EngineConstants::Presets::MOUNTAINS.warpStrength;
    float mountainPower = EngineConstants::Presets::MOUNTAINS.mountainPower;
    float octaves = 6.0f;
    float lacunarity = 2.0f;
    float persistence = 0.45f;
    float waterHeight = EngineConstants::Presets::MOUNTAINS.waterHeight;
    float fogDensity = 0.00065f;
    float debugMode = 0.0f;
    uint32_t presetType = EngineConstants::Presets::MOUNTAINS.type;
    int lodMode = -1;
    int viewRadius = EngineConstants::Terrain::DEFAULT_VIEW_RADIUS;
    bool wireframe = false;
    bool showFoliage = true;
    float foliageDensity = EngineConstants::Environment::DEFAULT_FOLIAGE_DENSITY;

    float timeOfDay = EngineConstants::Environment::DEFAULT_TIME_OF_DAY;
    bool timeCycleRunning = true;
    float timeCycleSpeed = EngineConstants::Environment::DEFAULT_CYCLE_SPEED;

    bool isDynamicLOD() const { return lodMode == -1; }

    float getViewDistance() const {
        return static_cast<float>(2 * viewRadius + 1) * CHUNK_SIZE * 0.5f;
    }

    void increaseViewDistance() {
        viewRadius = std::min(EngineConstants::Terrain::MAX_VIEW_RADIUS, viewRadius + 1);
        updateFog();
    }

    void decreaseViewDistance() {
        viewRadius = std::max(EngineConstants::Terrain::MIN_VIEW_RADIUS, viewRadius - 1);
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
        foliageDensity = std::min(EngineConstants::Environment::MAX_FOLIAGE_DENSITY, foliageDensity + 0.25f);
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

    void applyPreset(const EngineConstants::Presets::BiomePresetDef& def) {
        presetType = def.type;
        amplitude = def.amplitude;
        frequency = def.frequency;
        warpStrength = def.warpStrength;
        mountainPower = def.mountainPower;
        waterHeight = def.waterHeight;
        updateFog();
    }

    void applyMountains() {
        applyPreset(EngineConstants::Presets::MOUNTAINS);
    }

    void applyHills() {
        applyPreset(EngineConstants::Presets::HILLS);
    }

    void applyCanyons() {
        applyPreset(EngineConstants::Presets::CANYONS);
    }

    void applyIslands() {
        applyPreset(EngineConstants::Presets::ISLANDS);
    }

    void applyMultiBiome() {
        applyPreset(EngineConstants::Presets::MULTIBIOME);
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
