#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D fontTexture;

void main() {
    if (inUV.x < 0.0) {

        outColor = inColor;
    } else {

        float alpha = texture(fontTexture, inUV).r;
        outColor = vec4(inColor.rgb, inColor.a * alpha);
    }
}
