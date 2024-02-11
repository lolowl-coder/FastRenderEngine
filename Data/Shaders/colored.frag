#version 450

layout(location = 0) in float fragPosZ;
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColour;	//Final output colour (must also have location)	

const int keysCount = 5;
vec3 colorRamp[keysCount] =
{
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 1.0, 0.0),
	vec3(1.0, 0.0, 0.0),
	vec3(1.0, 0.0, 0.0)
};

float PI = 3.1415926535897932384626433832795;

void main()
{
	float factor = (fragPosZ / (2.0 * PI)) * 0.5 + 0.5;
	float denom = 1.0 / float(keysCount - 1);
	float tmp = factor / denom;
	float key;
	float t = modf(tmp, key);
	vec3 from = colorRamp[int(key)];
	vec3 to = colorRamp[int(key) + 1];
	vec3 color = mix(from, to, t);
	outColour = vec4(color, 1.0);
}