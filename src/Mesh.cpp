#include "Mesh.hpp"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
	VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices,
	int newTexId)
{
	indexCount = indices->size();
	vertexCount = vertices->size();
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	createVertexBuffer(transferQueue, transferCommandPool, vertices);
	createIndexBuffer(transferQueue, transferCommandPool, indices);

	model.model = glm::mat4(1.0f);
	texId = newTexId;
}

Mesh::~Mesh()
{
}

void Mesh::setModel(glm::mat4 newModel)
{
	model.model = newModel;
}

Model Mesh::getModel()
{
	return model;
}

int Mesh::getTexId()
{
	return texId;
}

int Mesh::getVertexCount()
{
	return vertexCount;
}

int Mesh::getIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
	return indexBuffer;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::destroyBuffers()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	//Size of buffer in bytes
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	//Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//Create buffer and allocate memory for it
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO VERTEX BUFFER
	void* data;		//1. Create pointer to a point in normal memory
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);	//Map the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);	//Copy memory from vertices vector to the point
	vkUnmapMemory(device, stagingBufferMemory);	//Unamp vertex buffer memory

	//Create destination vertex buffer for GPU memory
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

	//Copy staging buffer to vertex buffer on GPU
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	//Clean up staging buffer parts
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	//Get size of buffer for indices
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();
	
	//Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO INDEX BUFFER
	void* data;		//1. Create pointer to a point in normal memory
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);	//Map the buffer memory to that point
	memcpy(data, indices->data(), (size_t)bufferSize);	//Copy memory from indices vector to the point
	vkUnmapMemory(device, stagingBufferMemory);	//Unamp buffer memory

	//Create destination vertex buffer for GPU memory
	createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	//Copy staging buffer to vertex buffer on GPU
	copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

	//Clean up staging buffer parts
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}