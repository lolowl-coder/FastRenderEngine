#pragma once

#include "Mesh.hpp"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <vector>

namespace fre
{
	struct MainDevice;

	class MeshModel
	{
	public:
		MeshModel();
		MeshModel(std::vector<Mesh> newMeshList);

		size_t getMeshCount() const;
		const Mesh* getMesh(size_t index) const;
		const glm::mat4& getModelMatrix() const;
		void setModelMatrix(const glm::mat4& newModelMatrix);

		void destroyMeshModel(VkDevice logicalDevice);

		static std::vector<std::string> loadMaterials(const aiScene* scene);
		static std::vector<Mesh> loadNode(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			aiNode* node, const aiScene* scene, std::vector<int> matToTex);
		static Mesh loadMesh(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
			aiMesh * mesh, const aiScene* scene, std::vector<int> matToTex);

		~MeshModel();
	private:
		std::vector<Mesh> meshList;
		glm::mat4 modelMatrix;
	};
}