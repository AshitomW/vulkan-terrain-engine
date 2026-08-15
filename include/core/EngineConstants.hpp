#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace EngineConstants {

namespace Window {
    inline constexpr int DEFAULT_WIDTH = 1280;
    inline constexpr int DEFAULT_HEIGHT = 720;
    inline constexpr const char* TITLE = "Vulkan Procedural Terrain Engine";
}

namespace Camera {
    inline const glm::vec3 DEFAULT_POSITION = glm::vec3(0.0f, 65.0f, 0.0f);
    inline const glm::vec3 DEFAULT_FRONT = glm::vec3(0.0f, -0.2f, -1.0f);
    inline const glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    inline constexpr float DEFAULT_SPEED = 35.0f;
    inline constexpr float TURBO_MULTIPLIER = 3.5f;
    inline constexpr float MOUSE_SENSITIVITY = 0.12f;
    inline constexpr float FOV = 65.0f;
    inline constexpr float NEAR_PLANE = 0.1f;
    inline constexpr float FAR_PLANE = 4000.0f;
}

namespace Terrain {
    inline constexpr uint32_t GRID_RESOLUTION = 65;
    inline constexpr float CHUNK_SIZE = 128.0f;
    inline constexpr float CELL_SIZE = CHUNK_SIZE / static_cast<float>(GRID_RESOLUTION - 1);
    inline constexpr uint32_t NUM_LOD_LEVELS = 4;
    inline constexpr int DEFAULT_VIEW_RADIUS = 3;
    inline constexpr int MIN_VIEW_RADIUS = 1;
    inline constexpr int MAX_VIEW_RADIUS = 8;
    inline constexpr uint32_t DEFAULT_SEED = 1337;
}

namespace Environment {
    inline constexpr float DEFAULT_TIME_OF_DAY = 14.5f;
    inline constexpr float DEFAULT_CYCLE_SPEED = 0.20f;
    inline constexpr float DEFAULT_WATER_HEIGHT = -10.0f;
    inline constexpr float DEFAULT_FOLIAGE_DENSITY = 1.0f;
    inline constexpr float MAX_FOLIAGE_DENSITY = 3.0f;

    namespace SkyColors {
        inline const glm::vec3 NOON_ZENITH    = glm::vec3(0.025f, 0.090f, 0.280f);
        inline const glm::vec3 NOON_HORIZON   = glm::vec3(0.220f, 0.440f, 0.700f);
        inline const glm::vec3 NOON_SUN       = glm::vec3(1.0f, 0.95f, 0.85f);
        inline constexpr float NOON_SUN_INTENSITY = 1.35f;
        inline constexpr float NOON_AMBIENT   = 0.55f;

        inline const glm::vec3 SUNSET_ZENITH  = glm::vec3(0.030f, 0.045f, 0.180f);
        inline const glm::vec3 SUNSET_HORIZON = glm::vec3(0.850f, 0.320f, 0.080f);
        inline const glm::vec3 SUNSET_SUN     = glm::vec3(1.0f, 0.45f, 0.12f);
        inline constexpr float SUNSET_SUN_INTENSITY = 1.45f;
        inline constexpr float SUNSET_AMBIENT = 0.40f;

        inline const glm::vec3 DUSK_ZENITH    = glm::vec3(0.012f, 0.016f, 0.080f);
        inline const glm::vec3 DUSK_HORIZON   = glm::vec3(0.280f, 0.090f, 0.110f);
        inline const glm::vec3 DUSK_SUN       = glm::vec3(0.35f, 0.14f, 0.08f);
        inline constexpr float DUSK_SUN_INTENSITY = 0.35f;
        inline constexpr float DUSK_AMBIENT   = 0.25f;

        inline const glm::vec3 NIGHT_ZENITH   = glm::vec3(0.002f, 0.004f, 0.012f);
        inline const glm::vec3 NIGHT_HORIZON  = glm::vec3(0.006f, 0.012f, 0.030f);
        inline const glm::vec3 NIGHT_MOON     = glm::vec3(0.22f, 0.32f, 0.48f);
        inline constexpr float NIGHT_MOON_INTENSITY = 0.38f;
        inline constexpr float NIGHT_AMBIENT  = 0.15f;
    }
}

namespace Water {
    inline constexpr float DEFAULT_WAVE_AMPLITUDE = 0.60f;
    inline constexpr float DEFAULT_WAVE_SPEED = 1.0f;
    inline constexpr float DEFAULT_WATER_CLARITY = 0.15f;
    inline constexpr uint32_t WATER_GRID_RES = 160;
}

namespace Presets {
    struct BiomePresetDef {
        uint32_t type;
        float amplitude;
        float frequency;
        float warpStrength;
        float mountainPower;
        float waterHeight;
    };

    inline const BiomePresetDef MOUNTAINS  = {0, 115.0f, 1.0f, 0.85f, 1.80f, -10.0f};
    inline const BiomePresetDef HILLS      = {1,  38.0f, 1.0f, 0.40f, 0.50f, -10.0f};
    inline const BiomePresetDef CANYONS    = {2,  80.0f, 1.0f, 1.00f, 1.50f, -10.0f};
    inline const BiomePresetDef ISLANDS    = {3,  60.0f, 1.0f, 0.65f, 1.30f,   8.0f};
    inline const BiomePresetDef MULTIBIOME = {4, 100.0f, 1.0f, 0.80f, 1.60f,   0.0f};
}

}
