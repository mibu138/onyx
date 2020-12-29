#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 uv;

layout(location = 0) out vec3 outUv;

void main()
{
    vec3 scaledPos = (pos / 1000.0);
    scaledPos = scaledPos - vec3(0.5, 0.5, 0);
    scaledPos = scaledPos * 2;
    gl_Position = vec4(scaledPos, 1.0);
    outUv = uv;
}
