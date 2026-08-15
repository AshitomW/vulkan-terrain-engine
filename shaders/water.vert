#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out float outWaveHeight;
layout(location = 4) out float outFoamFactor;
layout(location = 5) out vec3 outUndisplacedPos;

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sunDir;
    vec4 sunColor;
    vec4 skyColorZenith;
    vec4 skyColorHorizon;
    vec4 terrainParams;
    vec4 biomeParams;
} ubo;

layout(push_constant) uniform WaterPC {
    vec4 waterParams1; // x: waterHeight, y: waveAmplitude, z: waveSpeed, w: time
    vec4 waterParams2; // x: clarity, y: causticsStrength, z: frequency, w: debugMode
    vec4 gridCenter;   // x: centerX, y: centerZ, z: gridRadius, w: unused
} pc;

struct GerstnerWave {
    vec2 dir;
    float amp;
    float wavelength;
    float speed;
    float steepness;
};

void evaluateGerstner(
    vec2 p,
    float time,
    float ampScale,
    float speedScale,
    out vec3 displacement,
    out vec3 normal,
    out float foam
) {
    const int NUM_WAVES = 4;
    GerstnerWave waves[NUM_WAVES] = GerstnerWave[](
        GerstnerWave(normalize(vec2(0.70, 0.70)), 0.38, 38.0, 1.05, 0.55),
        GerstnerWave(normalize(vec2(-0.45, 0.89)), 0.24, 20.0, 1.30, 0.50),
        GerstnerWave(normalize(vec2(0.88, -0.35)), 0.14, 10.5, 1.65, 0.45),
        GerstnerWave(normalize(vec2(-0.65, -0.75)), 0.08,  5.2, 2.10, 0.40)
    );

    vec3 disp = vec3(0.0);
    vec3 n = vec3(0.0, 1.0, 0.0);
    float jacobianSum = 0.0;

    for (int i = 0; i < NUM_WAVES; ++i) {
        float wAmp = waves[i].amp * ampScale;
        if (wAmp <= 0.0001) continue;

        float k = 6.2831853 / waves[i].wavelength;
        float c = sqrt(9.81 / k) * waves[i].speed * speedScale;
        vec2 d = waves[i].dir;
        float q = waves[i].steepness / (k * wAmp * float(NUM_WAVES) + 0.0001);
        q = clamp(q, 0.0, 0.85);

        float phase = k * dot(d, p) - (k * c) * time;
        float cosP = cos(phase);
        float sinP = sin(phase);

        disp.x += q * wAmp * d.x * cosP;
        disp.z += q * wAmp * d.y * cosP;
        disp.y += wAmp * sinP;

        float wa = k * wAmp;
        n.x -= d.x * wa * cosP;
        n.z -= d.y * wa * cosP;
        n.y -= q * wa * sinP;

        jacobianSum += q * wa * sinP;
    }

    displacement = disp;
    normal = normalize(n);
    foam = clamp(jacobianSum * 1.8 + (1.0 - normal.y) * 2.2, 0.0, 1.0);
}

void main() {
    float waterHeight = pc.waterParams1.x;
    float waveAmp = pc.waterParams1.y;
    float waveSpeed = pc.waterParams1.z;
    float time = pc.waterParams1.w;

    vec2 centerXZ = pc.gridCenter.xy;
    vec3 worldPos = vec3(inPosition.x + centerXZ.x, waterHeight, inPosition.z + centerXZ.y);
    outUndisplacedPos = worldPos;

    vec3 displacement;
    vec3 normal;
    float foam;
    evaluateGerstner(worldPos.xz, time, waveAmp, waveSpeed, displacement, normal, foam);

    worldPos += displacement;

    outWorldPos = worldPos;
    outNormal = normal;
    outUV = worldPos.xz * 0.05;
    outWaveHeight = displacement.y;
    outFoamFactor = foam;

    gl_Position = ubo.viewProj * vec4(worldPos, 1.0);
}
