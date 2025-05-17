#version 460

layout(location = 0) in vec2 fragTex;
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;	//Final output colour (must also have location)	

void main()
{
	//outColor = vec4(fragCol, 1.0);
	outColor = texture(textureSampler, fragTex);
}