#version 460
#extension GL_EXT_ray_tracing : enable

struct Payload
{
    vec3 radiance;
    vec3 albedo;
    vec3 normal;
    vec3 attenuation;
    vec3 rayOrigin;
    vec3 rayDir;
    uint rngState;
    int done;
    int depth;
};

layout(location = 0) rayPayloadInEXT Payload payload;

void main()
{
	payload.radiance = vec3(0.1) * payload.attenuation;
	payload.done = 1;
}