#version 450

layout(location = 0) in vec3 fragCol;	//Final output colour (must also have location)	
layout(location = 1) in vec2 fragTex;
layout(location = 2) in vec3 fragPos;
layout(set = 1, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 2, binding = 0) uniform sampler2D normalsSampler;

layout(push_constant) uniform Lighting {
	layout(offset = 64) vec4 cameraEye;
	layout(offset = 64 + 16) vec4 lightPos;
	layout(offset = 64 + 32) mat4 normalMatrix;
} lighting;

layout(location = 0) out vec4 outColour;	//Final output colour (must also have location)	

void main()
{
	//outColour = vec4(fragCol, 1.0);

	//diffuse
	vec4 tmp = texture(diffuseSampler, fragTex);
	vec3 diffuseColor = tmp.rgb;
	//diffuseColor = vec3(1.0);
	float alpha = tmp.a;
	vec3 normal = normalize(texture(normalsSampler, fragTex).xyz * 2.0 - 1.0);
	normal = normalize(transpose(inverse(mat3(lighting.normalMatrix))) * normal);
	//normal = vec3(0.0, 0.0, 1.0);
	vec3 fragLightDir = normalize(lighting.lightPos.xyz - fragPos);
	float diffuseFactor = max(0.0, dot(normal, fragLightDir));

	//specular
	vec3 specularColor = vec3(1.0);
	vec3 reflectedDir = reflect(-fragLightDir, normal);
	vec3 fragEyeDir = normalize(lighting.cameraEye.xyz - fragPos);
	float shininess = 32.0;
	float specularFactor = pow(max(0.0, dot(-fragEyeDir, reflectedDir)), shininess);
	outColour = vec4(
		diffuseColor * diffuseFactor +
		specularColor * specularFactor,
		alpha);
}