#version 450

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

layout(std430, set = 1, binding = 0) readonly buffer TerrainSSBO {
    vec4 data[];
};

layout(push_constant) uniform ChunkPC {
    vec4 chunkOffset;
    uvec4 lodParams;
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out float outHeight;
layout(location = 3) out float outSlope;
layout(location = 4) out vec2 outUV;
layout(location = 5) flat out uint outLOD;

void main() {
    uint vi = uint(gl_VertexIndex);

    uint lodGridRes = pc.lodParams.x;
    uint lodStep = pc.lodParams.y;
    uint totalGridRes = uint(pc.chunkOffset.w);

    uint lodX = vi % lodGridRes;
    uint lodZ = vi / lodGridRes;

    uint ssboX = min(lodX * lodStep, totalGridRes - 1u);
    uint ssboZ = min(lodZ * lodStep, totalGridRes - 1u);
    uint ssboIndex = ssboZ * totalGridRes + ssboX;

    vec4 vertexData = data[ssboIndex];
    float height = vertexData.x;
    vec3 normal = normalize(vertexData.yzw);

    float cellSize = pc.chunkOffset.z;
    vec2 chunkOrigin = pc.chunkOffset.xy;

    vec3 worldPos = vec3(
        chunkOrigin.x + float(ssboX) * cellSize,
        height,
        chunkOrigin.y + float(ssboZ) * cellSize
    );

    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

    outWorldPos = worldPos;
    outNormal = normal;
    outHeight = height;
    outSlope = slope;
    outUV = worldPos.xz * 0.05;
    outLOD = pc.lodParams.z;

    gl_Position = ubo.viewProj * vec4(worldPos, 1.0);
}
