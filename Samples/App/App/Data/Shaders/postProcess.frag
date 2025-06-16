#version 460

layout(set = 0, input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout(location = 0) out vec4 color;	//Final output colour (must also have location)	

void main()
{
	color = vec4(subpassLoad(inputColor).rgb, 1.0);
}