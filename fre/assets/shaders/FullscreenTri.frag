#version 450

layout(push_constant) uniform PushConstants
{
    float time;
} pc;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(abs(sin(pc.time)), 0.0, 0.0, 1.0);
}