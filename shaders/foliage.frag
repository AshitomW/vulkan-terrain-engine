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

vec3 toneMap(vec3 x) {
    return (x * (vec3(1.0) + x / 4.0)) / (vec3(1.0) + x);
}

void main() {
    vec3 N = normalize(inNormal);
    vec3 L = normalize(ubo.sunDir.xyz);
    vec3 D = normalize(inWorldPos - ubo.cameraPos.xyz);

    float fogDensity = ubo.terrainParams.y;

    float NdotL = max(dot(N, L), 0.0);
    float wrapLight = pow(clamp(dot(N, L) * 0.35 + 0.65, 0.0, 1.0), 1.8);
    vec3 directLight = ubo.sunColor.rgb * (NdotL * 0.70 + wrapLight * 0.30);

    vec3 skyAmb = ubo.skyColorZenith.rgb * ubo.skyColorZenith.a;
    vec3 groundAmb = ubo.skyColorHorizon.rgb * 0.35 * ubo.skyColorZenith.a;
    vec3 ambient = mix(groundAmb, skyAmb, N.y * 0.5 + 0.5);

    vec3 litColor = inColor * (ambient + directLight);

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
