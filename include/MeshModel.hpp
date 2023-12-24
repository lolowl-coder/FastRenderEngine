#pragma once

#include "Mesh.hpp"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <vector>

namespace fre
{
	class MeshModel
	{
	public:
		MeshModel();
		MeshModel(std::vector<Mesh> newMeshList);

		size_t getMeshCount();
		Mesh* getMesh(size_t index);
		glm::mat4 getModelMatrix();
		void setModelMatrix(const glm::mat4& newModelMatrix);

		void destroyMeshModel();

		static std::vector<std::string> loadMaterials(const aiScene* scene);
		static std::vector<Mesh> loadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			aiNode* node, const aiScene* scene, std::vector<int> matToTex);
		static Mesh loadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			aiMesh * mesh, const aiScene* scene, std::vector<int> matToTex);

		~MeshModel();
	private:
		std::vector<Mesh> meshList;
		glm::mat4 modelMatrix;
	};
}