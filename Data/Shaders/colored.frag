#version 450

layout(location = 0) in vec2 fragTex;

layout(location = 0) out vec4 outColour;	//Final output colour (must also have location)	

void main()
{
	outColour = vec4(1.0);
}