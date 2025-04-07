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

	//Multiple meshes in one model
	class MeshModel
	{
	public:
		using Ptr = std::shared_ptr<MeshModel>;
		using MeshList = std::vector<Mesh::Ptr>;

		MeshModel(const MeshList& newMeshList);
		~MeshModel();

		size_t getMeshCount() const;
		const Mesh::Ptr& getMesh(size_t index);
		const glm::mat4& getModelMatrix() const;
		void setModelMatrix(const glm::mat4& newModelMatrix);

		//Creates meshes from assimp node
		static std::vector<Mesh::Ptr> loadNode(aiNode* node, const aiScene* scene,
				BoundingBox3D& mn, uint32_t materialOffset);
		//Creates single mesh from assimp mesh
		static Mesh::Ptr loadMesh(aiMesh * mesh, BoundingBox3D& mn, uint32_t materialOffset);

		void setVisible(bool visible) { mVisible = visible; }
		bool isVisible() { return mVisible; }

	private:
		MeshList meshList;
		glm::mat4 modelMatrix;
		bool mVisible = true;
	};
}