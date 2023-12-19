#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.hpp"

struct Model
{
	glm::mat4 model;
};

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices,
		int newTexId);
	~Mesh();

	void setModel(glm::mat4 newModel);
	Model getModel();

	int getTexId();

	int getVertexCount();
	int getIndexCount();
	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	void destroyBuffers();
private:
	Model model;

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

