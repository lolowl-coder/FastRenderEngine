#include "MeshModel.hpp"
#include "Utilities.hpp"

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

	std::vector<std::string> MeshModel::loadMaterials(const aiScene* scene)
	{
		std::vector<std::string> textureList(scene->mNumMaterials);

		for (size_t i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* material = scene->mMaterials[i];

			textureList[i] = "";
			//Check for diffuse texture
			if (material->GetTextureCount(aiTextureType_DIFFUSE))
			{
				//Get texture file path
				aiString path;
				if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
				{
					//get file name
					int idx = std::string(path.data).rfind("\\");
					std::string fileName = std::string(path.data).substr(idx + 1);

					textureList[i] = fileName;
				}
			}
		}

		return textureList;
	}

	std::vector<Mesh> MeshModel::loadNode(
		const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, aiNode* node,
		const aiScene* scene, std::vector<int> matToTex)
	{
		std::vector<Mesh> meshList;

		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			meshList.push_back(
				loadMesh(mainDevice, transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, matToTex)
			);
		}

		//Go through each node attached to this node and load it, then append their meshes to this node's mesh lest
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			std::vector<Mesh> newList = loadNode(mainDevice, transferQueue, transferCommandPool, node->mChildren[i], scene, matToTex);
			meshList.insert(meshList.end(), newList.begin(), newList.end());
		}

		return meshList;
	}

	Mesh MeshModel::loadMesh(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiMesh * mesh, const aiScene* scene, std::vector<int> matToTex)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		//sync with mesh vertex numbers
		vertices.resize(mesh->mNumVertices);
		
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			//Set position
			vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
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

		//Iterate over indices through faces and copy across
		for (size_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace& face = mesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		Mesh newMesh = Mesh(mainDevice, transferQueue, transferCommandPool, &vertices, &indices, matToTex[mesh->mMaterialIndex]);

		return newMesh;
	}

	MeshModel::~MeshModel()
	{
	}
}