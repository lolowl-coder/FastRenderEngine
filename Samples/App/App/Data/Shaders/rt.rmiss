#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "Payload.h"
#include "Common.h"

layout(location = 0) rayPayloadInEXT Payload payload;
layout(set = 0, binding = 5, scalar) uniform DynamicData { DynamicDataBlock dynamicData; };

void main()
{
	payload.radiance = dynamicData.mBackgroundColor.xyz * payload.attenuation;
	payload.done = 1;
}