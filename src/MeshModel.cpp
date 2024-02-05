#include "MeshModel.hpp"
#include "Utilities.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <map>

namespace fre
{
	MeshModel::MeshModel(std::vector<Mesh> newMeshList)
	{
		meshList = newMeshList;
		modelMatrix = glm::mat4(1.0f);
	}

	void MeshModel::sync(const MainDevice& mainDevice,
		VkQueue transferQueue,
		VkCommandPool transferCommandPool)
	{
		for(auto& mesh : meshList)
		{
			mesh.sync(mainDevice, transferQueue, transferCommandPool);
		}
	}

	size_t MeshModel::getMeshCount() const
	{
		return meshList.size();
	}

	const Mesh* MeshModel::getMesh(size_t index) const
	{
		if (index >= meshList.size())
		{
			throw std::runtime_error("Attempted to access invalid Mesh Index!");
		}
		
		return &meshList[index];
	}

	const glm::mat4& MeshModel::getModelMatrix() const
	{
		return modelMatrix;
	}

	void MeshModel::setModelMatrix(const glm::mat4& newModelMatrix)
	{
		modelMatrix = newModelMatrix;
	}

	void MeshModel::destroyMeshModel(VkDevice logicalDevice)
	{
		for (auto& mesh : meshList)
		{
			mesh.destroyBuffers(logicalDevice);
		}
	}

	std::vector<Mesh> MeshModel::loadNode(aiNode* node, const aiScene* scene,
		glm::vec3& mn, glm::vec3& mx, uint32_t materialOffset)
	{
		std::vector<Mesh> meshList;

		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			meshList.push_back(
				loadMesh(scene->mMeshes[node->mMeshes[i]], mn, mx, materialOffset)
			);
		}

		//Go through each node attached to this node and load it,
		//then append their meshes to this node's mesh list
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			std::vector<Mesh> newList = loadNode(node->mChildren[i], scene, mn, mx, materialOffset);
			meshList.insert(meshList.end(), newList.begin(), newList.end());
		}

		return meshList;
	}

	Mesh MeshModel::loadMesh(aiMesh * mesh, glm::vec3& mn, glm::vec3& mx, uint32_t materialOffset)
	{
		//sync with mesh vertex numbers
		Mesh newMesh = Mesh(mesh->mMaterialIndex + materialOffset);
		
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			//Set position
			vertex.pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

			mn.x = std::min(mn.x, vertex.pos.x);
			mn.y = std::min(mn.y, vertex.pos.y);
			mn.z = std::min(mn.z, vertex.pos.z);
			
			mx.x = std::max(mx.x, vertex.pos.x);
			mx.y = std::max(mx.y, vertex.pos.y);
			mx.z = std::max(mx.z, vertex.pos.z);

			vertex.normal = glm::vec3(
				mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertex.tangent = glm::vec3(
				mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

			//Set tex coord (if exist)
			if (mesh->mTextureCoords[0])
			{
				vertex.tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			}
			else
			{
				vertex.tex = { 0.0f, 0.0f };
			}

			newMesh.addVertex(vertex);
		}

		std::cout << "mn: " << mn.x << " " << mn.y << " " << mn.z << std::endl;
		std::cout << "mx: " << mx.x << " " << mx.y << " " << mx.z << std::endl;

		//std::cout << "Tex coords components: " << mesh->mNumUVComponents[0] << std::endl;

		//Iterate over indices through faces and copy across
		for (size_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			if(face.mNumIndices != 3)
			{
				std::cout << "Non-triangle face encountered. Num indices: " <<
					face.mNumIndices << std::endl;
			}
			else
			{
				for (size_t j = 0; j < face.mNumIndices; j++)
				{
					newMesh.addIndex(face.mIndices[j]);
				}
			}
		}

		return newMesh;
	}

	MeshModel::~MeshModel()
	{
	}
}