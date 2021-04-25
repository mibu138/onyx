#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 uv;

layout(location = 0) out vec3 outUv;

layout(push_constant) uniform PC {
    layout(offset = 16)
    int   windowWidth;
    int   windowHeight;
} pc;

void main()
{
    vec3 scaledPos = vec3(pos.x / pc.windowWidth, pos.y / pc.windowHeight, 0.0);
    scaledPos = scaledPos - vec3(0.5, 0.5, 0);
    scaledPos = scaledPos * 2;
    gl_Position = vec4(scaledPos, 1.0);
    outUv = uv;
}
