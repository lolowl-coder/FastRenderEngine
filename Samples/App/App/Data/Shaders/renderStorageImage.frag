#version 460
#extension GL_EXT_shader_image_load_formatted : require

layout(set = 0, binding = 0) uniform readonly image2D myStorageImage;
layout(location = 0) out vec4 color;

void main()
{
    vec4 storedColor = imageLoad(myStorageImage, ivec2(gl_FragCoord.xy));
    color = vec4(storedColor.rgb, 1.0);
}