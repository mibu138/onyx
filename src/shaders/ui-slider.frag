#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

vec4 over(const vec4 a, const vec4 b)
{
    const vec3 color = a.rgb + b.rgb * (1. - a.a);
    const float alpha = a.a + b.a * (1. - a.a);
    return vec4(color, alpha);
}

layout(location = 0) in vec3 inUv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    int   width;
    int   height;
    float sliderPos;
} pc;

void main()
{
    vec2 st = inUv.st;
    float l = sdSegment(st, vec2(0.1, 0.5), vec2(0.9, 0.5));
    float c = sdCircle(
            (st - vec2(pc.sliderPos, 0.5)) * vec2(10, 1), 
            0.2);
    c = 1 - step(0, c);
    l = 1 - step(0.05, l);
    vec3 lineColor = vec3(0.1, 0.1, 0.1);
    vec3 circleColor = vec3(0.3, 0.5, 0.6);
    vec4 color = over(vec4(circleColor * c, c), vec4(lineColor * l, l));
    outColor = color;
}
