#version 460

layout(location = 0) in vec3 inUv;

layout(location = 0) out vec4 outColor;

float circleSDF(const vec2 center, const float radius, vec2 st)
{
    st -= center;
    return length(st) - radius;
}

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}
void main()
{
    vec2 st = inUv.st;
    st = 2 * (st - 0.5);
    float v = sdBox(st, vec2(0.9, 0.9));
    v = step(0, v);
    outColor = vec4(v, v, v, v);
}
