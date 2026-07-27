#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in float inHeight;
layout(location = 3) in float inSlope;
layout(location = 4) in vec2 inUV;
layout(location = 5) flat in uint inLOD;

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

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float detailNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

float fbmSimple(vec2 p) {
    float sum = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 3; ++i) {
        sum += amp * (detailNoise(p * freq) * 2.0 - 1.0);
        freq *= 2.0;
        amp *= 0.5;
    }
    return sum;
}

vec3 toneMap(vec3 x) {
    return (x * (vec3(1.0) + x / 4.0)) / (vec3(1.0) + x);
}

vec3 getMountainColor(float relH, float slope, float noise1, out float isSnow) {
    vec3 riverBed    = vec3(0.24, 0.20, 0.16);
    vec3 lushMeadow  = vec3(0.08, 0.24, 0.05);
    vec3 pineForest  = vec3(0.03, 0.14, 0.04);
    vec3 darkGranite = vec3(0.12, 0.13, 0.16);
    vec3 lightGranite= vec3(0.24, 0.26, 0.30);
    vec3 alpineSnow  = vec3(0.88, 0.92, 0.96);

    float rockStrata = sin(inWorldPos.y * 0.35 + noise1 * 1.5) * 0.5 + 0.5;
    vec3 cliffRock = mix(darkGranite, lightGranite, rockStrata);

    vec3 baseCol = riverBed;
    baseCol = mix(baseCol, lushMeadow, smoothstep(-0.02, 0.08, relH));
    baseCol = mix(baseCol, pineForest, smoothstep(0.14, 0.38, relH));
    baseCol = mix(baseCol, cliffRock, smoothstep(0.38, 0.65, relH));

    float cliffBlend = smoothstep(0.30, 0.60, slope + noise1 * 0.12);
    baseCol = mix(baseCol, cliffRock, cliffBlend);

    float snowAlt = smoothstep(0.55, 0.75, relH + noise1 * 0.08);
    float snowSlope = 1.0 - smoothstep(0.25, 0.55, slope);
    float snowFactor = clamp(snowAlt * snowSlope * 1.2, 0.0, 1.0);

    isSnow = snowFactor;
    return mix(baseCol, alpineSnow, snowFactor);
}

vec3 getHillsColor(float relH, float slope, float noise1) {
    vec3 valleyFloor = vec3(0.26, 0.20, 0.12);
    vec3 lushMeadow  = vec3(0.09, 0.26, 0.06);
    vec3 pastureGreen= vec3(0.12, 0.32, 0.08);
    vec3 forestGreen = vec3(0.04, 0.16, 0.05);
    vec3 richSoil    = vec3(0.18, 0.12, 0.08);

    vec3 col = valleyFloor;
    col = mix(col, lushMeadow, smoothstep(-0.02, 0.08, relH));
    col = mix(col, pastureGreen, smoothstep(0.10, 0.45, relH));
    col = mix(col, forestGreen, smoothstep(0.40, 0.85, relH));

    float slopeBlend = smoothstep(0.38, 0.70, slope + noise1 * 0.12);
    return mix(col, richSoil, slopeBlend * 0.75);
}

vec3 getCanyonColor(float relH, float slope, float noise1) {
    vec3 canyonFloor = vec3(0.28, 0.18, 0.10);
    vec3 burntTerra  = vec3(0.48, 0.14, 0.06);
    vec3 rustOchre   = vec3(0.58, 0.24, 0.08);
    vec3 goldenStrata= vec3(0.65, 0.36, 0.12);
    vec3 mesaScrub   = vec3(0.18, 0.22, 0.10);

    float strataLayer = sin(inWorldPos.y * 0.65 + noise1 * 1.8) * 0.5 + 0.5;
    vec3 strataCol = mix(burntTerra, goldenStrata, smoothstep(0.25, 0.75, strataLayer));
    strataCol = mix(strataCol, rustOchre, smoothstep(0.40, 0.60, strataLayer));

    vec3 col = mix(canyonFloor, strataCol, smoothstep(0.0, 0.08, relH));
    col = mix(col, mesaScrub, smoothstep(0.70, 0.95, relH));

    float cliffBlend = smoothstep(0.25, 0.55, slope);
    return mix(col, strataCol, cliffBlend);
}

vec3 getIslandColor(float relH, float slope, float noise1) {
    vec3 goldenSand   = vec3(0.68, 0.56, 0.32);
    vec3 coastalJungle= vec3(0.08, 0.28, 0.08);
    vec3 denseCanopy  = vec3(0.03, 0.15, 0.04);
    vec3 darkBasalt   = vec3(0.10, 0.11, 0.13);

    vec3 col = mix(goldenSand, coastalJungle, smoothstep(0.02, 0.12, relH));
    col = mix(col, denseCanopy, smoothstep(0.15, 0.60, relH));
    col = mix(col, darkBasalt, smoothstep(0.55, 0.95, relH));

    float cliffBlend = smoothstep(0.35, 0.65, slope);
    return mix(col, darkBasalt, cliffBlend * 0.88);
}

void main() {
    vec3 N = normalize(inNormal);
    vec3 L = normalize(ubo.sunDir.xyz);
    vec3 D = normalize(inWorldPos - ubo.cameraPos.xyz);
    vec3 V = -D;
    vec3 H = normalize(L + V);

    float waterHeight = ubo.terrainParams.x;
    float fogDensity = ubo.terrainParams.y;
    int debugMode = int(ubo.terrainParams.w);
    float amplitude = max(ubo.biomeParams.x, 15.0);
    uint preset = uint(ubo.biomeParams.y);

    if (debugMode == 1) {
        vec3 lodColors[4] = vec3[4](
            vec3(0.08, 0.85, 0.18),
            vec3(0.88, 0.75, 0.08),
            vec3(0.88, 0.38, 0.08),
            vec3(0.82, 0.08, 0.08)
        );
        vec3 col = lodColors[clamp(inLOD, 0u, 3u)];
        float ndotl = max(dot(N, L), 0.25);
        outColor = vec4(col * ndotl, 1.0);
        return;
    } else if (debugMode == 2) {
        outColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    } else if (debugMode == 3) {
        outColor = vec4(vec3(inSlope), 1.0);
        return;
    }

    float noise1 = detailNoise(inWorldPos.xz * 0.22);
    float noise2 = detailNoise(inWorldPos.xz * 1.40);
    float detail = (noise1 * 0.7 + noise2 * 0.3) * 0.06 - 0.03;

    float h = inHeight;
    float relH = (h - waterHeight) / amplitude;

    vec3 surfaceAlbedo;
    float isWater = 0.0;
    float isSnow = 0.0;

    if (h < waterHeight) {
        isWater = 1.0;
        float depth = waterHeight - h;
        vec3 deepSea   = vec3(0.01, 0.05, 0.18);
        vec3 shallowSea= vec3(0.04, 0.28, 0.36);
        vec3 shoreFoam = vec3(0.80, 0.90, 0.95);

        float depthFactor = clamp(depth / 7.0, 0.0, 1.0);
        surfaceAlbedo = mix(shallowSea, deepSea, depthFactor);
        if (depth < 0.5) {
            surfaceAlbedo = mix(shoreFoam, shallowSea, depth / 0.5);
        }
    }

    else if (preset == 0u) {
        surfaceAlbedo = getMountainColor(relH, inSlope, noise1, isSnow);
    } else if (preset == 1u) {
        surfaceAlbedo = getHillsColor(relH, inSlope, noise1);
    } else if (preset == 2u) {
        surfaceAlbedo = getCanyonColor(relH, inSlope, noise1);
    } else if (preset == 3u) {
        surfaceAlbedo = getIslandColor(relH, inSlope, noise1);
    } else {

        vec2 macroP = inWorldPos.xz * 0.00040;
        float continent = fbmSimple(macroP);
        float moisture  = fbmSimple(macroP + vec2(12.3, 45.6));

        float snowFlag = 0.0;
        vec3 colMtn    = getMountainColor(relH, inSlope, noise1, snowFlag);
        vec3 colHills  = getHillsColor(relH, inSlope, noise1);
        vec3 colCanyon = getCanyonColor(relH, inSlope, noise1);
        vec3 colIsland = getIslandColor(relH, inSlope, noise1);

        float wIsland   = smoothstep(0.05, -0.20, continent);
        float wHighland = smoothstep(-0.10, 0.25, continent);
        float wHills    = 1.0 - max(wIsland, wHighland);
        float wCanyon   = wHighland * smoothstep(-0.15, 0.20, moisture);
        float wMountain = wHighland * (1.0 - smoothstep(-0.15, 0.20, moisture));

        float totalWeight = wIsland + wHills + wMountain + wCanyon + 0.0001;
        surfaceAlbedo = (colIsland * wIsland + colHills * wHills + colMtn * wMountain + colCanyon * wCanyon) / totalWeight;

        if (wMountain > 0.35 && snowFlag > 0.4) {
            isSnow = snowFlag;
        }
    }

    surfaceAlbedo = clamp(surfaceAlbedo + detail * 0.05, 0.0, 1.0);

    float NdotL = max(dot(N, L), 0.0);
    float wrapLight = pow(clamp(dot(N, L) * 0.35 + 0.65, 0.0, 1.0), 1.8);
    vec3 directLight = ubo.sunColor.rgb * (NdotL * 0.70 + wrapLight * 0.30);

    float slopeAO = clamp(1.0 - inSlope * 0.25, 0.60, 1.0);
    float valleyAO = clamp(smoothstep(-10.0, 25.0, inHeight - waterHeight) * 0.25 + 0.75, 0.65, 1.0);
    float ao = slopeAO * valleyAO;

    vec3 skyAmb = ubo.skyColorZenith.rgb * ubo.skyColorZenith.a;
    vec3 groundAmb = ubo.skyColorHorizon.rgb * 0.35 * ubo.skyColorZenith.a;
    vec3 ambient = mix(groundAmb, skyAmb, N.y * 0.5 + 0.5) * ao;

    float specPower = isWater > 0.5 ? 120.0 : (isSnow > 0.5 ? 40.0 : 16.0);
    float specStrength = isWater > 0.5 ? 1.00 : (isSnow > 0.5 ? 0.40 : 0.02);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, specPower) * specStrength * (NdotL > 0.0 ? 1.0 : 0.0);

    vec3 litColor = surfaceAlbedo * (ambient + directLight) + ubo.sunColor.rgb * specular;

    float dist = length(ubo.cameraPos.xyz - inWorldPos);
    float fogFactor = 1.0 - exp(-pow(dist * fogDensity, 1.30));

    float elevation = clamp(D.y * 0.60 + 0.40, 0.0, 1.0);
    vec3 horizonAtmosphere = mix(ubo.skyColorHorizon.rgb, ubo.skyColorZenith.rgb, elevation);

    float sunDot = max(dot(D, L), 0.0);
    float sunGlare = pow(sunDot, 32.0) * 0.40;
    vec3 fogColor = horizonAtmosphere + ubo.sunColor.rgb * sunGlare;

    vec3 finalColor = mix(litColor, fogColor, clamp(fogFactor, 0.0, 1.0));

    finalColor = toneMap(finalColor * 1.15);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
