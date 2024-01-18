#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 colour;	//Final output colour (must also have location)	

layout(push_constant) uniform PushModel {
	vec2 nearFar;
};

#define FOG_NEAR 0.22
#define FOG_FAR 0.33

float linearizeDepth(float d, float zNear, float zFar)
{
    return zNear / (zFar + d * (zNear - zFar));
}

float linearStep(float mn, float mx, float value)
{
	return (value - mn) / (mx - mn);
}

void main()
{
	float depth = subpassLoad(inputDepth).r;
	float linearDepth = linearizeDepth(depth, nearFar.x, nearFar.y);
	float alpha = 1.0 - smoothstep(FOG_NEAR, FOG_FAR, linearDepth);
	colour = vec4(subpassLoad(inputColour).rgb, alpha);
}