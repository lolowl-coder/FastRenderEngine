#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec2 fragTex;

layout(set = 1, binding = 0) uniform sampler2D normalsSampler;
layout(set = 2, binding = 0) uniform sampler2D baseColorSampler;
layout(set = 3, binding = 0) uniform sampler2D metallicRoughnessSampler;

layout(push_constant) uniform Lighting {
	layout(offset = 64) vec4 cameraEye;
	layout(offset = 64 + 16) vec4 lightPos;
	layout(offset = 64 + 16 + 16) vec4 lightColor;
	layout(offset = 64 + 16 + 16 + 16) mat4 normalMatrix;
} lighting;

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;

//#define ROUGHNESS_PATTERN 1

vec3 materialcolor()
{
    vec4 tmp = texture(baseColorSampler, fragTex);
	return vec3(tmp.rgb);
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic)
{
	vec3 F0 = mix(vec3(0.04), materialcolor(), metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lighting.lightColor.rgb;
	}

	return color;
}

vec3 getNormal()
{
	//return fragNormal;
	vec3 n = normalize(fragNormal);
	vec3 t = normalize(fragTangent);
	vec3 b = normalize(cross(n, t));
	mat3 tbn = (mat3(t, b, n));
	vec3 normal = normalize(texture(normalsSampler, fragTex).xyz * 2.0 - 1.0);
	normal = tbn * normal;

	return normal;
}

// ----------------------------------------------------------------------------
void main()
{
	vec3 N = normalize(getNormal());
	vec3 V = normalize(lighting.cameraEye.xyz - fragPos);

    vec2 metallicRoughness = texture(metallicRoughnessSampler, fragTex).rg;
	float roughness = metallicRoughness.g;

	// Add striped pattern to roughness based on vertex position
#ifdef ROUGHNESS_PATTERN
	roughness = max(roughness, step(fract(fragPos.y * 2.02), 0.5));
#endif

	// Specular contribution
    vec3 L = normalize(lighting.lightPos.xyz - fragPos);
    float metallic = metallicRoughness.r;
    vec3 Lo = BRDF(L, V, N, metallic, roughness);

	// Combine with ambient
	vec3 color = materialcolor() * 0.02;
	color += Lo;

	// Gamma correct
	color = pow(color, vec3(0.4545));

	outColor = vec4(color, 1.0);
}