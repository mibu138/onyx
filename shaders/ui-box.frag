#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) in vec3 inUv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    int width;
    int height;
} pc;

void main()
{
    const float borderWidth  = 4.0 / pc.width;
    const float borderHeight = 4.0 / pc.height;
    vec2 st = inUv.st;
    st = 2 * (st - 0.5);
    float v = sdBox(st, vec2(1- borderWidth, 1 - borderHeight));
    v = step(0, v);
    const vec3 c = vec3(0.2, 0.2, 0.2);
    outColor = vec4(c, v);
}
