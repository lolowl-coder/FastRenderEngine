#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "Common.h"
#include "Constants.h"
#include "Payload.h"
#include "Rnd.h"

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
    if(dynamicData.mBackgroundType == 0)
    {
        uvec2 launchID = gl_LaunchIDEXT.xy;
        uvec2 launchSize = gl_LaunchSizeEXT.xy;
        float noiseFreq = dynamicData.mNoiseParams.x;
        float noisePow = dynamicData.mNoiseParams.y;
        float noiseOffset = dynamicData.mNoiseParams.z;
        float noiseScale = dynamicData.mNoiseParams.w;
        float n = min(10.0, pow(fbm(vec2(launchID.xy) / vec2(launchSize.xy) * noiseFreq), noisePow) * noiseScale + noiseOffset);
        float intensity = (1.0 - clamp(
            length(gl_LaunchIDEXT.xy - gl_LaunchSizeEXT.xy * 0.5) /
            (min(gl_LaunchSizeEXT.x, gl_LaunchSizeEXT.y) * 0.75), 0.0, 1.0)) ;
        payload.radiance = mix(vec3(0.0), vec3(dynamicData.mBackgroundColor.xyz * n), intensity);
	}
    else
    {
	    vec3 envColor = sampleEnvironment(payload.rayDir);
        payload.radiance = envColor;
    }
	payload.done = 1;
}