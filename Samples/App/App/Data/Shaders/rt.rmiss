#version 460
#extension GL_EXT_ray_tracing : enable

#include "Payload.h"

layout(location = 0) rayPayloadInEXT Payload payload;

void main()
{
	payload.radiance = vec3(0.01, 0.01, 0.03) * payload.attenuation;
	payload.done = 1;
}