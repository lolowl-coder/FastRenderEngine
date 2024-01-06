#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 colour;	//Final output colour (must also have location)	

void main()
{
	#if 0
		int xHalf = 800 / 2;
		if(gl_FragCoord.x > xHalf)
		{
			float lowerBound = 0.99;
			float upperBound = 1.0;
			
			float depth = subpassLoad(inputDepth).r;
			float depthColourScaled = 1.0f - ((depth - lowerBound) / (upperBound - lowerBound));
			
			colour = vec4(subpassLoad(inputColour).rgb * depthColourScaled, 1.0);
		}
	#else
		colour = subpassLoad(inputColour).rgba;
	#endif
}