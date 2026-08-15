#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

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

vec3 aces_film(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 N = normalize(inNormal);
    vec3 L = normalize(ubo.sunDir.xyz);
    vec3 D = normalize(inWorldPos - ubo.cameraPos.xyz);

    float dist = length(ubo.cameraPos.xyz - inWorldPos);
    float maxDist = max(ubo.biomeParams.w, 150.0);
    float distFactor = clamp(dist / maxDist, 0.0, 1.0);

    if (distFactor >= 0.94) {
        discard;
    }

    float NdotL = max(dot(N, L), 0.0);
    float wrapLight = pow(clamp(dot(N, L) * 0.35 + 0.65, 0.0, 1.0), 1.8);
    vec3 directLight = ubo.sunColor.rgb * (NdotL * 0.70 + wrapLight * 0.30);

    vec3 skyAmb = ubo.skyColorZenith.rgb * ubo.skyColorZenith.a;
    vec3 groundAmb = ubo.skyColorHorizon.rgb * 0.35 * ubo.skyColorZenith.a;
    vec3 ambient = mix(groundAmb, skyAmb, N.y * 0.5 + 0.5);

    vec3 litColor = inColor * (ambient + directLight);

    float waterHeight = ubo.terrainParams.x;
    float heightHaze = exp(-max(inWorldPos.y - waterHeight, 0.0) * 0.035) * 0.22;
    float valleyFog = heightHaze * clamp(dist / (maxDist * 0.4), 0.0, 1.0);

    float expFog = 1.0 - exp(-pow(distFactor * 1.7, 2.0));
    float edgeFade = smoothstep(0.70, 0.94, distFactor);
    float distFog = clamp(max(expFog, edgeFade), 0.0, 1.0);
    float fogFactor = clamp(distFog + valleyFog, 0.0, 1.0);

    float elevation = clamp(D.y * 0.75 + 0.25, 0.0, 1.0);
    vec3 horizonAtmosphere = mix(ubo.skyColorHorizon.rgb, ubo.skyColorZenith.rgb, elevation);

    float sunDot = max(dot(D, L), 0.0);
    float mie = (1.0 - 0.76 * 0.76) / pow(1.0 + 0.76 * 0.76 - 2.0 * 0.76 * sunDot, 1.5) * 0.07;
    float sunGlow = pow(sunDot, 16.0) * 0.25 + mie * 0.7;
    vec3 fogColor = horizonAtmosphere + ubo.sunColor.rgb * sunGlow * ubo.sunColor.a;

    vec3 finalColor = mix(litColor, fogColor, fogFactor);

    finalColor = aces_film(finalColor * 1.10);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
