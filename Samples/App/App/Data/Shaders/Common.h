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
	mat4 mViewInverse;
	mat4 mProjInverse;
	mat4 prevPV;
	mat4 PV;
	//x - main light intensity
	//y - lamp light intensity
	//z - enable GI
	//w - enable Area lights
	vec4 mLightingSettings;
	//xyz - world light position
	//w - light radius
	vec4 mLightPos;
	vec4 mLightColor;
	vec4 mBackgroundColor;
	//x - frequency
	//y - power
	//z - offset
	//w - amplitude
	vec4 mNoiseParams;
	//RT settings
	//mMaxRayDepth
	//mGiSamples
	//mEmissiveSamples
	//mLodDistanceRatio
	vec4 mRTSettings;
	float mFireflyThreshold;
	float mFlowMultiplier;
	int mDebugMode;
	int mAASamples;
	int mFrameIndex;
	float mFrameIndexRec;
	//0 - static color
	//1 - HDR env map
	int mBackgroundType;
	//Environment texture id
	int mEnvTexIndex;
	int mEnableToneMapping;
};

// DynamicDataBlock getters
float getMainLightIntensity(in DynamicDataBlock data)
{
    return data.mLightingSettings.x;
}

float getAmbientLightIntensity(in DynamicDataBlock data)
{
    return data.mLightingSettings.y;
}

bool getGIEnabled(in DynamicDataBlock data)
{
    return abs(data.mLightingSettings.z) > 0.01;
}

bool getAreaLightsEnabled(in DynamicDataBlock data)
{
    return abs(data.mLightingSettings.w) > 0.01;
}

#endif