#ifndef PBR_H
#define PBR_H

#include "Constants.h"

// PBR shading formula used in this project:
// Lo(x, V) = Le(x, V) + SumOverSamplesCount(Fr(x, L, V) * Li(x, L) * NdotL)
// Where:
// Lo(x, V) - Outgoing radiance (reflected light) at point x toward view direction V
// Le(x, V) - Emissive color
// Fr(x, L, V) - Bidirectional Reflectance Distribution Function (BRDF)
// Li(x, L) - Incoming light radiance. For directional light this is just the light radiance.
// (NdotL) - Simple angular term known from Blin-Phong lighting model
// x - surface point
// V - view direction (from surface point toward camera)
// L - light direction (from surface point toward light)
// N - surface normal at point x

// Math helpers
float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec3  saturate(vec3  v)
{
    return clamp(v, vec3(0.0), vec3(1.0));
}

// Trowbridge-Reitz GGX normal distribution
float D_GGX(float NdotH, float a)
{
	// Squared roughness
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (M_PI * d * d);
}

// Smith masking-shadowing using Schlick-GGX (Schlick-Beckmann) for both terms
float G_SchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    // k for direct lighting (correlated Smith)
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

// Fresnel-Schlick (with optional roughness variant for grazing)
vec3 F_Schlick(vec3 F0, float HdotV)
{
    float f = pow(1.0 - HdotV, 5.0);
    return F0 + (1.0 - F0) * f;
}

// --------------------------- BRDF core ---------------------------------------
struct PBRInputs
{
    vec3 N;        // world-space normal
    vec3 V;        // world-space view dir (from P toward camera), normalized
    vec3 L;        // world-space light dir (from P toward light), normalized
    vec3 radiance; // light radiance (RGB), already includes attenuation
    vec3 baseColor;
    float metallic;
    float roughness;
};

// PBR shading function
vec3 shadePBR(const PBRInputs I)
{
	// Halfway vector
    vec3 H = normalize(I.V + I.L);

    float NdotL = saturate(dot(I.N, I.L));
    float NdotV = saturate(dot(I.N, I.V));
    float NdotH = saturate(dot(I.N, H));
    float HdotV = saturate(dot(H, I.V));

    if(NdotL <= 0.0 || NdotV <= 0.0)
        return vec3(0.0);

    // BRDF = Kd * Idiffuse + Ks * Ispecular //I - integral
    // BRDF describes diffuse and specular contributions
    // Energy-conservation: Kd + Ks = 1

	// 1. Compute Ks * Ispecular
    
    // Base reflectevity of material (color when viewed at normal incidence)
    // For dielectric F0 ~ 0.04; metals use baseColor as F0
    vec3 F0 = mix(vec3(0.04), I.baseColor, I.metallic);

    // Compute Fresnel term
    vec3 F = F_Schlick(F0, HdotV);
    
    // perceptual -> alpha
    float a = I.roughness * I.roughness;
	// Normal distribution function
    float D = D_GGX(NdotH, a);
	// Geometry shadowing function
    float G = G_Smith(NdotV, NdotL, I.roughness);
	// Cook-Torrance specular term. Fresnel effect is already included in F
    vec3 spec = (D * G * F) / max(4.0 * NdotL * NdotV, 1e-4);

    // 2. Compute Kd * Idiffuse
    
    // Diffuse calculated by Lambert model, energy-conserving with metallic
    vec3 kd = (1.0 - F) * (1.0 - I.metallic);
	//FLambert = baseColor / PI * NdotL
	//NdotL is applied later
    vec3 diff = kd * I.baseColor / M_PI;
    
    //Lo(x, V) = Le(x, V) + SumOverSamplesCount(Fr(x, L, V) * Li(x, L) * NdotL)
    return
        // Fr(x, L, V)
        (diff + spec) *
        // Li(x, L)
        I.radiance *
		// Angular term
        NdotL;
}

#endif