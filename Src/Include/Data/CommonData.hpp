#pragma once

#include "Pointers.hpp"
#include "Utilities.hpp"

#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <assimp/scene.h>

namespace fre
{
	//Default vertex
	struct Vertex
	{
		glm::vec3 pos = glm::vec3(0.0f);
		glm::vec3 normal = glm::vec3(0.0f);
		glm::vec3 tangent = glm::vec3(0.0f);
		glm::vec2 tex = glm::vec2(0.0f);
	};

	//Lighting data for shader
	struct Lighting
	{
		glm::vec4 cameraEye = glm::vec4(0.0f);
		glm::vec4 lightPos = glm::vec4(0.0f);
		glm::vec4 lightDiffuseColor = glm::vec4(0.0f);
		glm::vec4 lightSpecularColor = glm::vec4(0.0f);
		glm::mat4 normalMatrix = glm::mat4(1.0f);
	};

	//Light
	struct Light
	{
		glm::vec3 mPosition = glm::vec3(100.0f);
		glm::vec3 mDiffuseColor = glm::vec3(1.0f);
		glm::vec3 mSpecularColor = glm::vec3(1.0f);
	};

    //Material. Minimal set of parameters
    struct Material
    {
        Material();

        bool hasTextureTypes(const std::vector<aiTextureType>& textureTypes) const;

        std::map<aiTextureType, uint32_t> mTextureIds;
        
        uint32_t mId = MAX(uint32_t);
        std::string mName;
        uint32_t mShaderId = MAX(uint32_t);
        std::string mShaderFileName;
		
		glm::vec4  mBaseColorFactor = glm::vec4(1.0f);      // RGBA
		float mMetallicFactor = 1.0f;       // [0,1]
		float mRoughnessFactor = 1.0f;      // [0,1]
		float mNormalScale = 1.0f;          // normal map scale (1 = as is)
		float mOcclusionStrength = 1.0f;    // [0,1]
		glm::vec3 mEmissiveFactor = glm::vec3(1.0f);
    };

	struct RTMaterialGPU
	{
		glm::vec4  mBaseColorFactor;      // RGBA
		float mMetallicFactor = 1.0f;       // [0,1]
		float mRoughnessFactor = 1.0f;      // [0,1]
		float mNormalScale = 1.0f;          // normal map scale (1 = as is)
		float mOcclusionStrength = 1.0f;    // [0,1]
		glm::vec3 mEmissiveFactor = glm::vec3(1.0f);       // RGB
		int mBaseColorMap = -1;         // -1 if none
		int mMetallicRoughnessMap = -1; // -1 if none (G=roughness, B=metallic)
		int mNormalMap = -1;            // -1 if none (tangent space)
		int mOcclusionMap = -1;         // -1 if none (R)
		int mEmissiveMap = -1;          // -1 if none (RGB)
	};

	struct DynamicData
	{
		glm::mat4 mViewInverse = glm::mat4(1.0f);
		glm::mat4 mProjInverse = glm::mat4(1.0f);
		float mLightIntensity = 4.5f;
		float mAmbient = 0.03f;
	};

	struct RTMeshCPU
	{
		std::vector<Vertex> mVertices;
		//Vertex indices of triangles
		std::vector<uint32_t> mIndices;
		//Material per triangle
		std::vector<int32_t> mMatIndices;
	};

	struct RTMeshGPU
	{
		VkDeviceAddress mVertices;
		VkDeviceAddress mIndices;
		int mMaterialIndex;
	};

	struct EmissiveTri
	{
		glm::vec3 v0, v1, v2;   // v0, and edges (v1-v0), (v2-v0)
		glm::vec2 uv0, uv1, uv2;// tex coords
		float area;             // 0.5 * length(cross(e1, e2))
        int matIndex;
	};
}
