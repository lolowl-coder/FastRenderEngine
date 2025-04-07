#include "MeshModel.hpp"

#include "Log.hpp"
#include "Utilities.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <map>

using namespace glm;

namespace fre
{
	MeshModel::MeshModel(const MeshList& newMeshList)
	{
		meshList = newMeshList;
		modelMatrix = mat4(1.0f);
	}

	size_t MeshModel::getMeshCount() const
	{
		return meshList.size();
	}

	const Mesh::Ptr& MeshModel::getMesh(size_t index)
	{
		if (index >= meshList.size())
		{
			throw std::runtime_error("Attempted to access invalid Mesh Index!");
		}
		
		return meshList[index];
	}

	const mat4& MeshModel::getModelMatrix() const
	{
		return modelMatrix;
	}

	void MeshModel::setModelMatrix(const mat4& newModelMatrix)
	{
		modelMatrix = newModelMatrix;
	}

	std::vector<Mesh::Ptr> MeshModel::loadNode(aiNode* node, const aiScene* scene,
		BoundingBox3D& bb, uint32_t materialOffset)
	{
		std::vector<Mesh::Ptr> meshList;

		for (size_t i = 0; i < node->mNumMeshes; i++)
		{
			meshList.push_back(
				loadMesh(scene->mMeshes[node->mMeshes[i]], bb, materialOffset)
			);
		}

		//Go through each node attached to this node and load it,
		//then append their meshes to this node's mesh list
		for (size_t i = 0; i < node->mNumChildren; i++)
		{
			std::vector<Mesh::Ptr> newList = loadNode(node->mChildren[i], scene, bb, materialOffset);
			meshList.insert(meshList.end(), newList.begin(), newList.end());
		}

		return meshList;
	}

	Mesh::Ptr MeshModel::loadMesh(aiMesh * mesh, BoundingBox3D& bb, uint32_t materialOffset)
	{
		//sync with mesh vertex numbers
		Mesh::Ptr newMesh(new Mesh(mesh->mMaterialIndex + materialOffset));
		BoundingBox3D thisBB;
		Mesh::Vertices vertices;
		vertices.resize(mesh->mNumVertices * sizeof(Vertex));
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			auto* vertex = static_cast<Vertex*>((void*)&vertices[i * sizeof(Vertex)]);
			//Set position
			vertex->pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

			thisBB.mMin.x = std::min(thisBB.mMin.x, vertex->pos.x);
			thisBB.mMin.y = std::min(thisBB.mMin.y, vertex->pos.y);
			thisBB.mMin.z = std::min(thisBB.mMin.z, vertex->pos.z);
			
			thisBB.mMax.x = std::max(thisBB.mMax.x, vertex->pos.x);
			thisBB.mMax.y = std::max(thisBB.mMax.y, vertex->pos.y);
			thisBB.mMax.z = std::max(thisBB.mMax.z, vertex->pos.z);

			if(mesh->mNormals)
			{
				vertex->normal = vec3(
					mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}
			if(mesh->mTangents)
			{
				vertex->tangent = vec3(
					mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			}

			//Set tex coord (if exist)
			if (mesh->mTextureCoords[0])
			{
				vertex->tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			}
			else
			{
				vertex->tex = { 0.0f, 0.0f };
			}
		}

		newMesh->setVertices(vertices, sizeof(Vertex));

		newMesh->setBoundingBox(bb);

		bb.mMin.x = std::min(bb.mMin.x, thisBB.mMin.x);
		bb.mMin.y = std::min(bb.mMin.y, thisBB.mMin.y);
		bb.mMin.z = std::min(bb.mMin.z, thisBB.mMin.z);
		bb.mMax.x = std::max(bb.mMax.x, thisBB.mMax.x);
		bb.mMax.y = std::max(bb.mMax.y, thisBB.mMax.y);
		bb.mMax.z = std::max(bb.mMax.z, thisBB.mMax.z);

		//LOG_TRACE("Mesh model BB: mn: {}, {}, {}", bb.mMin.x, bb.mMin.y, bb.mMin.z);
		//LOG_TRACE("Mesh model BB: mx: {}, {}, {}", bb.mMax.x, bb.mMax.y, bb.mMax.z);

		//std::cout << "Tex coords components: " << mesh->mNumUVComponents[0] << std::endl;

		//Iterate over indices through faces and copy across
		Mesh::Indices indices;
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
					indices.push_back(face.mIndices[j]);
				}
			}
		}
		newMesh->setIndices(indices);

		return newMesh;
	}

	MeshModel::~MeshModel()
	{
	}
}