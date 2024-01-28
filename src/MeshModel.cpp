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

	std::vector<std::map<aiTextureType, std::string>> MeshModel::loadMaterials(
		const aiScene* scene, aiTextureType texturesLoadMask)
	{
		std::vector<std::map<aiTextureType, std::string>> result(scene->mNumMaterials);
		std::set<std::string> unifqueTextureFilePaths;
		for (uint32_t m = 0; m < scene->mNumMaterials; m++)
		{
			aiMaterial* material = scene->mMaterials[m];

			//Collect all textures
			for(uint32_t i = 1; i < aiTextureType_UNKNOWN && (i & texturesLoadMask); i++)
			{
				//Get texture file path
				aiString path;
				if (material->GetTexture(static_cast<aiTextureType>(i), 0, &path) == AI_SUCCESS)
				{
					//get file name
					auto idx = std::string(path.data).rfind("\\");
					if(idx == -1)
					{
						idx = std::string(path.data).rfind("/");
					}
					std::string fileName = std::string(path.data).substr(idx + 1);

					result[m][static_cast<aiTextureType>(i)] = fileName;
				}
			}
		}

		return result;
	}

	std::vector<Mesh> MeshModel::loadNode(
		const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, aiNode* node,
		const aiScene* scene, glm::vec3& mn, glm::vec3& mx)
	{
		std::vector<Mesh> meshList;

		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			meshList.push_back(
				loadMesh(mainDevice, transferQueue, transferCommandPool,
					scene->mMeshes[node->mMeshes[i]], scene, mn, mx)
			);
		}

		//Go through each node attached to this node and load it,
		//then append their meshes to this node's mesh list
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			std::vector<Mesh> newList = loadNode(mainDevice, transferQueue, transferCommandPool,
				node->mChildren[i], scene, mn, mx);
			meshList.insert(meshList.end(), newList.begin(), newList.end());
		}

		return meshList;
	}

	Mesh MeshModel::loadMesh(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, aiMesh * mesh, const aiScene* scene,
		glm::vec3& mn, glm::vec3& mx)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		//sync with mesh vertex numbers
		vertices.resize(mesh->mNumVertices);
		
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			//Set position
			vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

			mn.x = std::min(mn.x, vertices[i].pos.x);
			mn.y = std::min(mn.y, vertices[i].pos.y);
			mn.z = std::min(mn.z, vertices[i].pos.z);
			
			mx.x = std::max(mx.x, vertices[i].pos.x);
			mx.y = std::max(mx.y, vertices[i].pos.y);
			mx.z = std::max(mx.z, vertices[i].pos.z);

			//Set tex coord (if exist)
			if (mesh->mTextureCoords[0])
			{
				vertices[i].tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			}
			else
			{
				vertices[i].tex = { 0.0f, 0.0f };
			}

			//Set white for now
			vertices[i].col = { 1.0f, 1.0f, 1.0f };
		}

		//std::cout << "mn: " << mn.x << " " << mn.y << " " << mn.z << std::endl;
		//std::cout << "mx: " << mx.x << " " << mx.y << " " << mx.z << std::endl;

		std::cout << "Tex coords components: " << mesh->mNumUVComponents[0] << std::endl;

		//Iterate over indices through faces and copy across
		for (size_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			if(face.mNumIndices != 3)
			{
				std::cout << "num indices: " << face.mNumIndices << std::endl;
			}
			else
			{
				for (size_t j = 0; j < face.mNumIndices; j++)
				{
					indices.push_back(face.mIndices[j]);
				}
			}
		}

		Mesh newMesh = Mesh(mainDevice, transferQueue, transferCommandPool, &vertices,
			&indices, mesh->mMaterialIndex);

		return newMesh;
	}

	MeshModel::~MeshModel()
	{
	}
}