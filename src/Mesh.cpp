#include "Mesh.hpp"

namespace fre
{
	static uint32_t gMeshId = 0;

	Mesh::Mesh()
	: mId(gMeshId++)
	{
	}

	Mesh::Mesh(uint32_t materialId)
	: mId(gMeshId++)
	{
		mModelMatrix = glm::mat4(1.0f);
		mMaterialId = materialId;
	}

	Mesh::~Mesh()
	{
	}

	uint32_t Mesh::getId() const
	{
		return mId;
	}

	void Mesh::addVertex(const Vertex& vertex)
	{
		mVertices.push_back(vertex);
	}

	void Mesh::addIndex(const uint32_t index)
	{
		mIndices.push_back(index);
	}

	void Mesh::setModelMatrix(glm::mat4 modelMatrix)
	{
		mModelMatrix = modelMatrix;
	}

	const glm::mat4& Mesh::getModelMatrix() const
	{
		return mModelMatrix;
	}

	int Mesh::getMaterialId() const
	{
		return mMaterialId;
	}
	
	void Mesh::setMaterialId(int materialId)
	{
		mMaterialId = materialId;
	}

	int Mesh::getVertexCount() const
	{
		return mVertices.size();
	}

	int Mesh::getIndexCount() const
	{
		return mIndices.size();
	}

	const void* Mesh::getVertexData() const
	{
		return mVertices.data();
	}

	const void* Mesh::getIndexData() const
	{
		return mIndices.data();
	}
}