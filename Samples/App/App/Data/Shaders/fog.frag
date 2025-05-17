#version 460

layout(set = 0, input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout(set = 1, binding = 0) uniform sampler2D inputDepth;
//layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 color;	//Final output colour (must also have location)	

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
	//float depth = subpassLoad(inputDepth).r;
	//float linearDepth = linearizeDepth(depth, nearFar.x, nearFar.y);
	//float alpha = 1.0 - smoothstep(FOG_NEAR, FOG_FAR, linearDepth);
	//color = vec4(subpassLoad(inputColor).rgb, alpha);
	color = vec4(subpassLoad(inputColor).rgb, 1.0);
	
	//color.rgb = mix(vec3(1.0, 0.0, 1.0), color.rgb, smoothstep(0.0, 3.0, abs(gl_FragCoord.x - 1920.0 * 0.5)));

	//float depth = texture(inputDepth, gl_FragCoord.xy / vec2(1920.0, 1080.0)).r;
	//color = vec4(vec3(float(depth < 1.0)), 1.0);
}