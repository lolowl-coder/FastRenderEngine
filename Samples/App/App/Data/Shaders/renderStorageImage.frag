#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "Common.h"

layout(set = 0, binding = 0) uniform readonly image2D myStorageImage;
layout(set = 0, binding = 1, scalar) uniform DynamicData { DynamicDataBlock dynamicData; };

layout(location = 0) out vec4 color;

vec3 toneMapFilmic(vec3 x)
{
    x = max(vec3(0.0), x - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

void main()
{
    vec3 storedColor = imageLoad(myStorageImage, ivec2(gl_FragCoord.xy)).rgb;
    
    const float EV = dynamicData.mBackgroundColor.w;
    const float exposure = pow(2.0, EV);
    storedColor *= exposure;
    if(dynamicData.mEnableToneMapping == 1)
    {
        storedColor = toneMapFilmic(storedColor);
        storedColor = pow(storedColor, vec3(1.0 / 2.2));
    }

    color = vec4(storedColor.rgb, 1.0);
}