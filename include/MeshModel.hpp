#pragma once

#include "Mesh.hpp"
#include "Material.hpp"

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <map>
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

		static std::vector<std::map<aiTextureType, std::string>> loadMaterials(const aiScene* scene,
			aiTextureType texturesLoadMask);
		static std::vector<Mesh> loadNode(const MainDevice& mainDevice, VkQueue transferQueue,
			VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene,
			glm::vec3& mn, glm::vec3& mx);
		static Mesh loadMesh(const MainDevice& mainDevice, VkQueue transferQueue,
			VkCommandPool transferCommandPool, aiMesh * mesh, const aiScene* scene,
			glm::vec3& mn, glm::vec3& mx);

		~MeshModel();

	private:
		std::vector<Material> mMaterials;
		std::vector<Mesh> meshList;
		glm::mat4 modelMatrix;
	};
}