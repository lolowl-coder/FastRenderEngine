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
		const Mesh& getMesh(size_t index) const;
		const glm::mat4& getModelMatrix() const;
		void setModelMatrix(const glm::mat4& newModelMatrix);

		static std::vector<Mesh> loadNode(aiNode* node, const aiScene* scene, glm::vec3& mn,
			glm::vec3& mx, uint32_t materialOffset);
		static Mesh loadMesh(aiMesh * mesh, glm::vec3& mn, glm::vec3& mx, uint32_t materialOffset);

		~MeshModel();

	private:
		std::vector<Mesh> meshList;
		glm::mat4 modelMatrix;
	};
}