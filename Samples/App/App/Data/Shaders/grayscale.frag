#version 450

layout(location = 0) in vec2 fragTex;
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;	//Final output colour (must also have location)	

vec4 lineColors[] = { vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), vec4(0.0, 0.0, 1.0, 1.0), vec4(0.0, 1.0, 1.0, 1.0) };

void main()
{
	outColor = vec4(vec3(texture(textureSampler, fragTex).r), 1.0);
}