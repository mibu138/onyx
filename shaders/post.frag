#version 460

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;

void main()
{
    vec4 tex = texture(image, uv).rgba;
    //vec4 uvColor = vec4(uv, 0.0, 1.0);
    outColor = tex;
    //outColor = vec4(uv, 0, 1.0);
}
