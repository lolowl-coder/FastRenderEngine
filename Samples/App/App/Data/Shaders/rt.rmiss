#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "Payload.h"
#include "Common.h"
#include "Constants.h"

layout(location = 0) rayPayloadInEXT Payload payload;
layout(set = 0, binding = 5, scalar) uniform DynamicData { DynamicDataBlock dynamicData; };
// Scene textures
layout(set = 0, binding = 8) uniform sampler2D textures[];

vec2 dirToUV(vec3 dir)
{
    float theta = atan(dir.z, dir.x);
    float phi = acos(clamp(dir.y, -1.0, 1.0));
    float u = (theta / (2.0 * M_PI)) + 0.5;
    float v = phi / M_PI;
    return vec2(u, v);
}

vec3 sampleEnvironment(vec3 dir)
{
    vec2 uv = dirToUV(normalize(dir));
    return texture(textures[dynamicData.mEnvTexIndex], uv).rgb;
}

void main()
{
	vec3 envColor = sampleEnvironment(payload.rayDir);
    if(dynamicData.mBackgroundType == 0)
    {
        payload.radiance = dynamicData.mBackgroundColor.xyz;// *payload.attenuation;
	}
    else
    {
        payload.radiance = envColor;// *payload.attenuation;
    }
	payload.done = 1;
}