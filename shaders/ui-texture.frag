#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "common.glsl"

layout(location = 0) in vec3 inUv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    int texId;
} pc;

layout(set = 0, binding = 0) uniform usampler2D textures[1];

void main()
{
    vec2 st = inUv.st;
    uint v = texture(textures[0], st).r;
    float H = float(v) / 255.0;
    outColor = vec4(1, 1, 1, H);
}
