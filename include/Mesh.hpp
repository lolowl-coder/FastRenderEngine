#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utilities.hpp"

#include <limits>
#include <vector>

namespace fre
{
	class Mesh
	{
	public:
		Mesh();
		Mesh(uint32_t materialId);
		~Mesh();

		uint32_t getId() const;

		void addVertex(const Vertex& vertex);
		void addIndex(const uint32_t index);

		void setModelMatrix(glm::mat4 newModelMatrix);
		const glm::mat4& getModelMatrix() const;

		int getMaterialId() const;

		int getVertexCount() const;
		int getIndexCount() const;

		const void* getVertexData() const;
		const void* getIndexData() const;
	private:
		uint32_t mId = std::numeric_limits<uint32_t>::max();
		uint32_t mMaterialId = std::numeric_limits<uint32_t>::max();

		glm::mat4 mModelMatrix;

		std::vector<Vertex> mVertices;
		std::vector<uint32_t> mIndices;
	};
}