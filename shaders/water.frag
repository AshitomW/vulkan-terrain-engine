#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in float inWaveHeight;
layout(location = 4) in float inFoamFactor;
layout(location = 5) in vec3 inUndisplacedPos;

layout(location = 0) out vec4 outColor;

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

uint hashU(uvec2 p, uint seed) {
    uint h = seed;
    h ^= p.x + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= p.y + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

vec2 hash2(vec2 p, uint seed) {
    uvec2 ip = uvec2(ivec2(floor(p)) + 100000);
    uint h1 = hashU(ip, seed);
    uint h2 = hashU(ip + uvec2(137u, 269u), seed);
    return vec2(
        float(h1 & 0x00FFFFFFu) / 16777215.0,
        float(h2 & 0x00FFFFFFu) / 16777215.0
    ) * 2.0 - 1.0;
}

float gradientNoise(vec2 p, uint seed) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    vec2 g00 = hash2(i + vec2(0.0, 0.0), seed);
    vec2 g10 = hash2(i + vec2(1.0, 0.0), seed);
    vec2 g01 = hash2(i + vec2(0.0, 1.0), seed);
    vec2 g11 = hash2(i + vec2(1.0, 1.0), seed);

    float n00 = dot(g00, f - vec2(0.0, 0.0));
    float n10 = dot(g10, f - vec2(1.0, 0.0));
    float n01 = dot(g01, f - vec2(0.0, 1.0));
    float n11 = dot(g11, f - vec2(1.0, 1.0));

    return mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
}

float fbm(vec2 p, uint seed, int octaves, float lacunarity, float persistence) {
    float sum = 0.0;
    float amp = 1.0;
    float maxAmp = 0.0;
    float freq = 1.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * gradientNoise(p * freq, seed + uint(i * 31));
        maxAmp += amp;
        freq *= lacunarity;
        amp *= persistence;
    }
    return sum / maxAmp;
}

float evaluatePresetHeight(vec2 worldPos, uint presetType, float baseFreq, float amp, float warpStrength, float mountainPower, uint seed) {
    if (presetType == 0u) {
        float mFreq = baseFreq * 2.2;
        vec2 p = worldPos * mFreq;
        vec2 q = vec2(
            fbm(p * 0.5, seed + 11u, 4, 2.0, 0.5),
            fbm(p * 0.5 + vec2(5.2, 1.3), seed + 22u, 4, 2.0, 0.5)
        );
        vec2 warped = p + warpStrength * q;
        float chain = fbm(warped * 0.35, seed + 33u, 4, 2.0, 0.5);
        float chainMask = smoothstep(-0.30, 0.35, chain);

        float r1 = 1.0 - abs(gradientNoise(warped * 0.75, seed + 101u));
        float r2 = 1.0 - abs(gradientNoise(warped * 1.60, seed + 102u));
        float r3 = 1.0 - abs(gradientNoise(warped * 3.30, seed + 103u));
        float r4 = 1.0 - abs(gradientNoise(warped * 6.80, seed + 104u));
        float peakStructure = pow((r1 * 0.50 + r2 * 0.30 + r3 * 0.14 + r4 * 0.06), 2.1) * 2.2;

        float valley = abs(gradientNoise(warped * 0.45 + vec2(2.3, 7.1), seed + 200u));
        float valleyCarve = smoothstep(0.04, 0.60, valley);

        float lakeBasin = fbm(warped * 0.25, seed + 250u, 3, 2.0, 0.5);
        float lakeDip = smoothstep(-0.4, 0.2, lakeBasin);

        float h = mix(chain * 0.15 - 0.05, peakStructure * mountainPower * valleyCarve, chainMask * lakeDip);
        return (h * 0.88 + 0.04) * amp;
    } else if (presetType == 1u) {
        float hFreq = baseFreq * 4.2;
        vec2 p = worldPos * hFreq;
        vec2 q = vec2(
            gradientNoise(p * 0.35, seed + 10u),
            gradientNoise(p * 0.35 + vec2(4.1, 2.7), seed + 20u)
        );
        vec2 warped = p + warpStrength * 0.35 * q;

        float hill1 = sin(warped.x * 0.65) * cos(warped.y * 0.65) * 0.25;
        float hill2 = (gradientNoise(warped * 0.75, seed + 30u) * 0.5 + 0.5);
        float hill3 = (gradientNoise(warped * 1.50, seed + 40u) * 0.5 + 0.5) * 0.35;
        float hill4 = (gradientNoise(warped * 3.00, seed + 50u) * 0.5 + 0.5) * 0.12;
        float hill5 = gradientNoise(warped * 6.00, seed + 60u) * 0.04;

        float river = abs(gradientNoise(warped * 0.25 + vec2(1.2, 8.4), seed + 70u));
        float riverCarve = smoothstep(0.02, 0.25, river);

        float rollingMounds = pow(hill2 + hill3 + hill4 + hill1, 1.30) * 0.50 + hill5;
        return ((rollingMounds * riverCarve) * 0.85 + 0.08) * amp;
    } else if (presetType == 2u) {
        float cFreq = baseFreq * 2.8;
        vec2 p = worldPos * cFreq;
        vec2 q = vec2(
            fbm(p * 0.55, seed + 15u, 3, 2.0, 0.5),
            fbm(p * 0.55 + vec2(7.3, 1.9), seed + 25u, 3, 2.0, 0.5)
        );
        vec2 warped = p + warpStrength * 0.9 * q;

        float baseMesa = fbm(warped * 0.5, seed + 50u, 4, 2.0, 0.5);
        float numSteps = 8.0;
        float stepped = floor(baseMesa * numSteps) / numSteps;
        float frac = fract(baseMesa * numSteps);
        float terracedMesa = stepped + smoothstep(0.0, 0.28, frac) / numSteps;

        float chasm1 = abs(fbm(warped * 0.70, seed + 70u, 4, 2.0, 0.5));
        float chasm2 = abs(fbm(warped * 1.35 + vec2(3.1, 5.7), seed + 80u, 3, 2.0, 0.5));
        float gorge = min(chasm1, chasm2);
        float gorgeCarve = smoothstep(0.04, 0.35, gorge);

        float buttes = pow(1.0 - abs(gradientNoise(warped * 1.7, seed + 90u)), 3.2) * 0.30;
        return ((terracedMesa * gorgeCarve + buttes) * 0.85 + 0.06) * amp;
    } else {
        float iFreq = baseFreq * 2.6;
        vec2 p = worldPos * iFreq;
        vec2 q = vec2(
            gradientNoise(p * 0.5, seed + 12u),
            gradientNoise(p * 0.5 + vec2(3.7, 8.2), seed + 24u)
        );
        vec2 warped = p + warpStrength * 0.55 * q;

        float oceanBase = fbm(warped * 0.45, seed + 10u, 4, 2.0, 0.5);
        float islandPeak = pow(1.0 - abs(gradientNoise(warped * 1.25, seed + 60u)), 2.0);
        float volcanoCrater = abs(gradientNoise(warped * 2.5, seed + 70u)) * 0.25;

        float islandMask = smoothstep(-0.06, 0.35, oceanBase);
        return mix(-0.25, (islandPeak * 1.3 - volcanoCrater) * 1.25, islandMask) * amp;
    }
}

float evaluateContinuousHeight(vec2 worldPos) {
    float baseFreq = 0.004;
    float amp = max(ubo.biomeParams.x, 15.0);
    float warpStrength = 0.85;
    float mountainPower = 1.8;
    uint presetType = uint(ubo.biomeParams.y);
    uint seed = uint(ubo.biomeParams.z);

    if (presetType < 4u) {
        return evaluatePresetHeight(worldPos, presetType, baseFreq, amp, warpStrength, mountainPower, seed);
    }

    vec2 macroP = worldPos * 0.00045;
    float continent = fbm(macroP, seed + 500u, 4, 2.0, 0.5);
    float moisture  = fbm(macroP + vec2(12.3, 45.6), seed + 600u, 4, 2.0, 0.5);

    float wOcean = smoothstep(0.0, -0.25, continent);
    float wHighland = smoothstep(-0.05, 0.25, continent);
    float wHills = 1.0 - max(wOcean, wHighland);

    float wCanyon = wHighland * smoothstep(-0.1, 0.2, moisture);
    float wMountain = wHighland * (1.0 - smoothstep(-0.1, 0.2, moisture));

    float hMtn = evaluatePresetHeight(worldPos, 0u, baseFreq, amp * 1.10, warpStrength, mountainPower, seed);
    float hHill = evaluatePresetHeight(worldPos, 1u, baseFreq, amp * 0.40, warpStrength * 0.5, mountainPower * 0.6, seed);
    float hCanyon = evaluatePresetHeight(worldPos, 2u, baseFreq, amp * 0.85, warpStrength, mountainPower, seed);
    float hIsland = evaluatePresetHeight(worldPos, 3u, baseFreq, amp * 0.65, warpStrength, mountainPower, seed);

    float totalWeight = wOcean + wHills + wMountain + wCanyon + 0.0001;
    return (hIsland * wOcean + hHill * wHills + hMtn * wMountain + hCanyon * wCanyon) / totalWeight;
}

vec3 calculateMicroNormals(vec2 p, float time, float speed) {
    vec2 p1 = p * 0.45 + vec2(time * 0.45 * speed, time * 0.25 * speed);
    vec2 p2 = p * 1.15 - vec2(time * 0.30 * speed, -time * 0.50 * speed);
    vec2 p3 = p * 2.80 + vec2(-time * 0.60 * speed, time * 0.70 * speed);

    float n1 = gradientNoise(p1, 991u);
    float n2 = gradientNoise(p2, 992u);
    float n3 = gradientNoise(p3, 993u);

    float d1x = gradientNoise(p1 + vec2(0.08, 0.0), 991u) - n1;
    float d1y = gradientNoise(p1 + vec2(0.0, 0.08), 991u) - n1;
    float d2x = gradientNoise(p2 + vec2(0.08, 0.0), 992u) - n2;
    float d2y = gradientNoise(p2 + vec2(0.0, 0.08), 992u) - n2;
    float d3x = gradientNoise(p3 + vec2(0.08, 0.0), 993u) - n3;
    float d3y = gradientNoise(p3 + vec2(0.0, 0.08), 993u) - n3;

    vec2 slope = (vec2(d1x, d1y) * 0.45 + vec2(d2x, d2y) * 0.35 + vec2(d3x, d3y) * 0.20) * 2.2;
    return normalize(vec3(-slope.x, 1.0, -slope.y));
}

vec3 aces_film(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    float waterHeight = pc.waterParams1.x;
    float waveAmp = pc.waterParams1.y;
    float waveSpeed = pc.waterParams1.z;
    float time = pc.waterParams1.w;

    vec3 V = normalize(ubo.cameraPos.xyz - inWorldPos);
    vec3 L = normalize(ubo.sunDir.xyz);

    vec3 macroNormal = normalize(inNormal);
    vec3 microNormal = calculateMicroNormals(inWorldPos.xz, time, waveSpeed);
    vec3 N = normalize(macroNormal * 0.65 + microNormal * 0.35);

    float terrainFloorH = evaluateContinuousHeight(inWorldPos.xz);
    float waterDepth = max(inWorldPos.y - terrainFloorH, 0.0);

    // Fresnel reflectance (Schlick approximation)
    float NdotV = max(dot(N, V), 0.0);
    float R0 = 0.0204;
    float fresnel = R0 + (1.0 - R0) * pow(1.0 - NdotV, 5.0);
    fresnel = clamp(fresnel, 0.04, 0.95);

    // Ray-traced Sky / Sun reflection
    vec3 R = reflect(-V, N);
    float reflElev = clamp(R.y * 0.65 + 0.35, 0.0, 1.0);
    vec3 skyRefl = mix(ubo.skyColorHorizon.rgb, ubo.skyColorZenith.rgb, reflElev);

    float sunReflDot = max(dot(R, L), 0.0);
    float sunGlint = pow(sunReflDot, 256.0) * 3.5 + pow(sunReflDot, 32.0) * 0.8;
    vec3 reflectionColor = skyRefl + ubo.sunColor.rgb * sunGlint;

    // Ambient and Direct lighting on water body
    vec3 skyAmb = ubo.skyColorZenith.rgb * ubo.skyColorZenith.a;
    vec3 groundAmb = ubo.skyColorHorizon.rgb * 0.35 * ubo.skyColorZenith.a;
    vec3 ambient = mix(groundAmb, skyAmb, N.y * 0.5 + 0.5);

    float NdotL = max(dot(N, L), 0.0);
    float wrapLight = pow(clamp(dot(N, L) * 0.35 + 0.65, 0.0, 1.0), 1.8);
    vec3 directLight = ubo.sunColor.rgb * (NdotL * 0.70 + wrapLight * 0.30);
    vec3 totalIllum = ambient + directLight;

    // Physical water body & Beer-Lambert light extinction
    vec3 deepWater = vec3(0.008, 0.040, 0.160);
    vec3 shallowWater = vec3(0.040, 0.280, 0.360);
    vec3 waterBodyColor = mix(shallowWater, deepWater, smoothstep(0.5, 6.0, waterDepth)) * (totalIllum + 0.02);

    // Specular highlight
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, 320.0) * 3.5 + pow(NdotH, 48.0) * 0.7;

    vec3 waterColor = mix(waterBodyColor, reflectionColor, fresnel);
    if (L.y > -0.05) {
        waterColor += ubo.sunColor.rgb * specular;
    }

    // Natural shoreline froth
    float shoreNoise = gradientNoise(inWorldPos.xz * 2.5 + vec2(time * 0.5, -time * 0.35), 777u) * 0.5 + 0.5;
    float shoreFringe = smoothstep(0.20, 0.02, waterDepth) * smoothstep(0.35, 0.85, shoreNoise) * 0.40;

    // Subtle wave crest foam
    float crestFoam = smoothstep(0.70, 0.95, inFoamFactor) * smoothstep(0.40, 0.80, shoreNoise) * 0.35;
    float totalFoam = clamp(shoreFringe + crestFoam, 0.0, 0.60);

    vec3 foamColor = vec3(0.92, 0.96, 1.0) * (ambient * 0.5 + directLight * 0.6);
    waterColor = mix(waterColor, foamColor, totalFoam);

    // Atmospheric distance fog matching terrain and sky
    float dist = length(ubo.cameraPos.xyz - inWorldPos);
    float maxDist = max(ubo.biomeParams.w, 150.0);

    float distFactor = clamp(dist / maxDist, 0.0, 1.0);
    float expFog = 1.0 - exp(-pow(distFactor * 1.7, 2.0));
    float edgeFade = smoothstep(0.70, 0.94, distFactor);
    float fogFactor = clamp(max(expFog, edgeFade), 0.0, 1.0);

    vec3 D = -V;
    float elevation = clamp(D.y * 0.75 + 0.25, 0.0, 1.0);
    vec3 horizonAtmosphere = mix(ubo.skyColorHorizon.rgb, ubo.skyColorZenith.rgb, elevation);

    float sunDot = max(dot(D, L), 0.0);
    float mie = (1.0 - 0.76 * 0.76) / pow(1.0 + 0.76 * 0.76 - 2.0 * 0.76 * sunDot, 1.5) * 0.07;
    float sunGlow = pow(sunDot, 16.0) * 0.25 + mie * 0.7;
    vec3 fogColor = horizonAtmosphere + ubo.sunColor.rgb * sunGlow * ubo.sunColor.a;

    vec3 finalColor = mix(waterColor, fogColor, fogFactor);

    finalColor = aces_film(finalColor * 1.10);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    // Alpha: transparent in shallows, smoothly opaque towards horizon fog
    float baseAlpha = clamp(0.40 + fresnel * 0.45 + smoothstep(0.1, 2.5, waterDepth) * 0.45 + totalFoam * 0.3, 0.15, 0.95);
    float alpha = mix(baseAlpha, 1.0, smoothstep(0.65, 0.92, distFactor));
    outColor = vec4(finalColor, alpha);
}
