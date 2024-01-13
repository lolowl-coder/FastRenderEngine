#include "Mesh.hpp"

namespace fre
{
	Mesh::Mesh()
	{
	}

	Mesh::Mesh(
		const MainDevice& mainDevice,
		VkQueue transferQueue,
		VkCommandPool transferCommandPool,
		std::vector<Vertex>* vertices,
		std::vector<uint32_t>* indices,
		int newTexId)
	{
		indexCount = indices->size();
		vertexCount = vertices->size();
		createVertexBuffer(mainDevice, transferQueue, transferCommandPool, vertices);
		createIndexBuffer(mainDevice, transferQueue, transferCommandPool, indices);

		modelMatrix = glm::mat4(1.0f);
		texId = newTexId;
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::setModelMatrix(glm::mat4 newModelMatrix)
	{
		modelMatrix = newModelMatrix;
	}

	glm::mat4 Mesh::getModelMatrix()
	{
		return modelMatrix;
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

	void Mesh::destroyBuffers(VkDevice logicalDevice)
	{
		vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
		vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
		vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
		vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
	}

	void Mesh::createVertexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
	{
		//Size of buffer in bytes
		VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		//Create buffer and allocate memory for it
		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO VERTEX BUFFER
		void* data;		//1. Create pointer to a point in normal memory
		vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);	//Map the vertex buffer memory to that point
		memcpy(data, vertices->data(), (size_t)bufferSize);	//Copy memory from vertices vector to the point
		vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);	//Unamp vertex buffer memory

		//Create destination vertex buffer for GPU memory
		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
	}

	void Mesh::createIndexBuffer(const MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
	{
		//Get size of buffer for indices
		VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();
		
		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO INDEX BUFFER
		void* data;		//1. Create pointer to a point in normal memory
		vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);	//Map the buffer memory to that point
		memcpy(data, indices->data(), (size_t)bufferSize);	//Copy memory from indices vector to the point
		vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);	//Unamp buffer memory

		//Create destination vertex buffer for GPU memory
		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
	}
}