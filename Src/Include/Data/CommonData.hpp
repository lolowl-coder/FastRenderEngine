#pragma once

#include "Pointers.hpp"
#include "Utilities.hpp"

#include <map>
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

        std::map<aiTextureType, uint32_t> mTextureIds;
        float mShininess = 1.0;
        bool hasTextureTypes(const std::vector<aiTextureType>& textureTypes) const;
        uint32_t mId = MAX(uint32_t);
        uint32_t mShaderId = MAX(uint32_t);
        std::string mShaderFileName;
    };

	struct RTCamera
	{
		glm::mat4 mViewInverse;
		glm::mat4 mProjInverse;
	};

	struct RTMeshCPU
	{
		std::vector<Vertex> mVertices;
		//Vertex indices of triangles
		std::vector<uint32_t> mIndices;
		//Material per triangle
		std::vector<int32_t> mMatIndices;
	};

	struct RTMaterialGPU
	{
		glm::vec3 mSpecular{ 0.7f, 0.7f, 0.7f };
		float mShininess{ 0.f };
        int mDiffuseMap = MAX(int);
        int mNormalMap = MAX(int);
	};

	struct RTMeshGPU
	{
		VkDeviceAddress mVertices;
		VkDeviceAddress mIndices;
		int mMaterialIndex;
	};
}
