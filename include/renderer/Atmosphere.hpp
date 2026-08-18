#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include "core/EngineConstants.hpp"

namespace Atmosphere {

struct State {
    glm::vec3 sunDir;
    glm::vec3 activeLightDir;
    glm::vec3 sunColor;
    glm::vec3 skyZenith;
    glm::vec3 skyHorizon;
    float ambientIntensity = 0.0f;
    float dayFactor = 0.0f;
    float starFactor = 0.0f;
    float sinElevation = 0.0f;
    glm::vec4 skyClearColor{0.0f, 0.0f, 0.0f, 1.0f};
};

inline float smoothstep(float edge0, float edge1, float x) {
    float val = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return val * val * (3.0f - 2.0f * val);
}

inline State compute(float timeOfDay) {
    using namespace EngineConstants::Environment::SkyColors;

    State s;
    float sunAngle = ((timeOfDay - 6.0f) / 12.0f) * 3.141592653589793f;
    float sinElev = std::sin(sunAngle);
    float cosElev = std::cos(sunAngle);
    s.sinElevation = sinElev;

    s.sunDir = glm::normalize(glm::vec3(cosElev * 0.80f, sinElev, 0.35f));
    glm::vec3 moonDir = glm::normalize(glm::vec3(-cosElev * 0.80f, -sinElev, -0.35f));

    if (sinElev >= 0.15f) {
        float f = smoothstep(0.15f, 0.70f, sinElev);
        s.skyZenith = glm::mix(SUNSET_ZENITH, NOON_ZENITH, f);
        s.skyHorizon = glm::mix(SUNSET_HORIZON, NOON_HORIZON, f);
        s.sunColor = glm::mix(SUNSET_SUN * SUNSET_SUN_INTENSITY, NOON_SUN * NOON_SUN_INTENSITY, f);
        s.ambientIntensity = glm::mix(SUNSET_AMBIENT, NOON_AMBIENT, f);
        s.dayFactor = 1.0f;
        s.starFactor = 0.0f;
    } else if (sinElev >= 0.0f) {
        float f = smoothstep(0.0f, 0.15f, sinElev);
        s.skyZenith = glm::mix(DUSK_ZENITH, SUNSET_ZENITH, f);
        s.skyHorizon = glm::mix(SUNSET_HORIZON, SUNSET_HORIZON, f);
        s.sunColor = glm::mix(DUSK_SUN * DUSK_SUN_INTENSITY, SUNSET_SUN * SUNSET_SUN_INTENSITY, f);
        s.ambientIntensity = glm::mix(DUSK_AMBIENT, SUNSET_AMBIENT, f);
        s.dayFactor = glm::mix(0.35f, 1.0f, f);
        s.starFactor = 0.0f;
    } else if (sinElev >= -0.18f) {
        float f = smoothstep(-0.18f, 0.0f, sinElev);
        s.skyZenith = glm::mix(NIGHT_ZENITH, DUSK_ZENITH, f);
        s.skyHorizon = glm::mix(NIGHT_HORIZON, DUSK_HORIZON, f);
        s.sunColor = glm::mix(NIGHT_MOON * NIGHT_MOON_INTENSITY, DUSK_SUN * DUSK_SUN_INTENSITY, f);
        s.ambientIntensity = glm::mix(NIGHT_AMBIENT, DUSK_AMBIENT, f);
        s.dayFactor = glm::mix(0.0f, 0.35f, f);
        s.starFactor = 1.0f - f;
    } else {
        s.skyZenith = NIGHT_ZENITH;
        s.skyHorizon = NIGHT_HORIZON;
        s.sunColor = NIGHT_MOON * NIGHT_MOON_INTENSITY;
        s.ambientIntensity = NIGHT_AMBIENT;
        s.dayFactor = 0.0f;
        s.starFactor = 1.0f;
    }

    if (sinElev >= -0.05f) {
        float blend = smoothstep(-0.05f, 0.10f, sinElev);
        s.activeLightDir = glm::normalize(glm::mix(moonDir, s.sunDir, blend));
    } else {
        s.activeLightDir = moonDir;
    }

    if (sinElev > 0.15f) {
        s.skyClearColor = glm::vec4(0.025f, 0.090f, 0.280f, 1.0f);
    } else if (sinElev > -0.15f) {
        float f = (sinElev + 0.15f) / 0.30f;
        s.skyClearColor = glm::vec4(
            0.002f * (1.0f - f) + 0.025f * f,
            0.004f * (1.0f - f) + 0.090f * f,
            0.012f * (1.0f - f) + 0.280f * f,
            1.0f
        );
    } else {
        s.skyClearColor = glm::vec4(0.002f, 0.004f, 0.012f, 1.0f);
    }

    return s;
}

}