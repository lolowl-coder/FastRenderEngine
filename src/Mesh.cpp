#include "Mesh.hpp"

namespace fre
{
	static uint32_t gMeshId = 0;

	Mesh::Mesh()
		: mId(gMeshId++)
		, mGeneratedVerticesCount(0)
	{
	}

	Mesh::Mesh(uint32_t materialId)
		: mId(gMeshId++)
		, mMaterialId(materialId)
		, mGeneratedVerticesCount(0)
	{
		
	}

	Mesh::~Mesh()
	{
	}

	uint32_t Mesh::getId() const
	{
		return mId;
	}

	void Mesh::setVertices(const Vertices& vertices, uint32_t vertexSize)
	{
		mVertices = vertices;
		mVertexSize = vertexSize;
	}

	void Mesh::setIndices(const Indices& index)
	{
		mIndices = index;
	}

	uint32_t Mesh::getVertexSize() const
	{
		return mVertexSize;
	}

	uint32_t Mesh::getVertexCount() const
	{
		return mVertexSize > 0 ? static_cast<int>(mVertices.size() / mVertexSize) : 0;
	}

	uint32_t Mesh::getIndexCount() const
	{
		return static_cast<int>(mIndices.size());
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