#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.hpp"

namespace fre
{
	class Mesh
	{
	public:
		Mesh();
		Mesh(
			const MainDevice& mainDevice,
			VkQueue transferQueue,
			VkCommandPool transferCommandPool,
			std::vector<Vertex>* vertices,
			std::vector<uint32_t>* indices,
			int newTexId);
		~Mesh();

		void setModelMatrix(glm::mat4 newModelMatrix);
		const glm::mat4& getModelMatrix() const;

		int getTexId() const;

		int getVertexCount() const;
		int getIndexCount() const;
		VkBuffer getVertexBuffer() const;
		VkBuffer getIndexBuffer() const;
		void destroyBuffers(VkDevice logicalDevice);
	private:
		glm::mat4 modelMatrix;

		int texId;

		int vertexCount;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		int indexCount;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		void createVertexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
		void createIndexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
	};
}