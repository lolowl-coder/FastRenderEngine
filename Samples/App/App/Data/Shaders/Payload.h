#ifndef _PAYLOAD_
#define _PAYLOAD_

struct Payload
{
	vec3 radiance;
	vec3 albedo;
	vec3 normal;
	vec3 attenuation;
	vec3 rayOrigin;
	vec3 rayDir;
	//uint rngState;
	int done;
	int depth;
	float weight;
};

#endif
