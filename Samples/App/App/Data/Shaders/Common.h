#ifndef _COMMON_
#define _COMMON_

struct Material
{
	vec4  mBaseColorFactor;      // RGBA
	float mMetallicFactor;       // [0,1]
	float mRoughnessFactor;      // [0,1]
	float mNormalScale;          // normal map scale (1 = as is)
	float mOcclusionStrength;    // [0,1]
	vec3  mEmissiveFactor;       // RGB
	int mBaseColorTex;         // -1 if none
	int mMetallicRoughnessTex; // -1 if none (G=roughness, B=metallic)
	int mNormalTex;            // -1 if none (tangent space)
	int mOcclusionTex;         // -1 if none (R)
	int mEmissiveTex;          // -1 if none (RGB)
};

struct Vertex
{
	vec3 mPos;
	vec3 mNormal;
	vec3 mTangent;
	vec2 mTC;
};

struct Mesh
{
	uint64_t mVertices;
	uint64_t mIndices;
	int mMaterialIndex;
};

struct EmissiveTriangle
{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	float area;
	int matIndex;
};

struct DynamicDataBlock
{
	mat4 viewInverse;
	mat4 projInverse;
	float mainLightIntensity;
	float ambient;
};

#endif
