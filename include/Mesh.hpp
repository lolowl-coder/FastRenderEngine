#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.hpp"

namespace fre
{
	struct ModelMatrix
	{
		glm::mat4 modelMatrix;
	};

	class Mesh
	{
	public:
		Mesh();
		Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
			VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices,
			int newTexId);
		~Mesh();

		void setModelMatrix(glm::mat4 newModelMatrix);
		ModelMatrix getModelMatrix();

		int getTexId();

		int getVertexCount();
		int getIndexCount();
		VkBuffer getVertexBuffer();
		VkBuffer getIndexBuffer();
		void destroyBuffers();
	private:
		ModelMatrix model;

		int texId;

		int vertexCount;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		int indexCount;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		VkPhysicalDevice physicalDevice;
		VkDevice device;

		void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
		void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
	};
}