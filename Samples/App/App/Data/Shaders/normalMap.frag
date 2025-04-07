#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec2 fragTex;
layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 2, binding = 0) uniform sampler2D normalsSampler;

layout(push_constant) uniform Lighting {
	layout(offset = 64) vec4 cameraEye;
	layout(offset = 64 + 16) vec4 lightPos;
	layout(offset = 64 + 16 + 16) vec4 lightColor;
	layout(offset = 64 + 16 + 16 + 16) mat4 normalMatrix;
} lighting;

layout(location = 0) out vec4 outColor;	//Final output colour (must also have location)	

vec3 getNormal()
{
	vec3 n = normalize(fragNormal);
	vec3 t = normalize(fragTangent);
	vec3 b = normalize(cross(n, t));
	mat3 tbn = (mat3(t, b, n));
	vec3 normal = normalize(texture(normalsSampler, fragTex).xyz * 2.0 - 1.0);
	normal = tbn * normal;

	return normal;
}

void main()
{
	//diffuse
	vec4 tmp = texture(diffuseSampler, fragTex);
	vec3 diffuseColor = tmp.rgb;
	float alpha = tmp.a;
	vec3 normal = getNormal();
	vec3 fragLightDir = normalize(lighting.lightPos.xyz - fragPos);
	float diffuseFactor = max(0.0, dot(normal, fragLightDir));

	//specular
	vec3 specularColor = vec3(1.0);
	vec3 reflectedDir = reflect(-fragLightDir, normal);
	vec3 fragEyeDir = normalize(lighting.cameraEye.xyz - fragPos);
	float shininess = lighting.lightPos.w;
	float specularFactor = pow(max(0.0, dot(fragEyeDir, reflectedDir)), shininess);
	outColor = vec4(
		diffuseColor * diffuseFactor * lighting.lightColor.rgb +
		specularColor * specularFactor * lighting.lightColor.rgb,
		alpha);
}