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

		void addVertex(const Vertex& vertex);
		void addIndex(const uint32_t index);

		void sync(
			const MainDevice& mainDevice,
			VkQueue transferQueue,
			VkCommandPool transferCommandPool);

		void setModelMatrix(glm::mat4 newModelMatrix);
		const glm::mat4& getModelMatrix() const;

		int getMaterialId() const;

		int getVertexCount() const;
		int getIndexCount() const;
		VkBuffer getVertexBuffer() const;
		VkBuffer getIndexBuffer() const;
		void destroyBuffers(VkDevice logicalDevice);
	private:
		glm::mat4 mModelMatrix;

		uint32_t mMaterialId = std::numeric_limits<uint32_t>::max();

		std::vector<Vertex> mVertices;
		std::vector<uint32_t> mIndices;

		VkBuffer mVertexBuffer;
		VkDeviceMemory mVertexBufferMemory;

		VkBuffer mIndexBuffer;
		VkDeviceMemory mIndexBufferMemory;

		void createVertexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool);
		void createIndexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool);
	};
}