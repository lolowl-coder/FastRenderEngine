#version 460

layout(location = 0) in vec2 fragTex;
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(textureSampler, fragTex);
}