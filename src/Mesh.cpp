#include "Mesh.hpp"

namespace fre
{
	Mesh::Mesh()
	{
	}

	Mesh::Mesh(uint32_t materialId)
	{
		mModelMatrix = glm::mat4(1.0f);
		mMaterialId = materialId;
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::addVertex(const Vertex& vertex)
	{
		mVertices.push_back(vertex);
	}

	void Mesh::addIndex(const uint32_t index)
	{
		mIndices.push_back(index);
	}

	void Mesh::sync(
		const MainDevice& mainDevice,
		VkQueue transferQueue,
		VkCommandPool transferCommandPool)
	{
		createVertexBuffer(mainDevice, transferQueue, transferCommandPool);
		createIndexBuffer(mainDevice, transferQueue, transferCommandPool);
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

	int Mesh::getVertexCount() const
	{
		return mVertices.size();
	}

	int Mesh::getIndexCount() const
	{
		return mIndices.size();
	}

	VkBuffer Mesh::getIndexBuffer() const
	{
		return mIndexBuffer;
	}

	VkBuffer Mesh::getVertexBuffer() const
	{
		return mVertexBuffer;
	}

	void Mesh::destroyBuffers(VkDevice logicalDevice)
	{
		vkDestroyBuffer(logicalDevice, mVertexBuffer, nullptr);
		vkFreeMemory(logicalDevice, mVertexBufferMemory, nullptr);
		vkDestroyBuffer(logicalDevice, mIndexBuffer, nullptr);
		vkFreeMemory(logicalDevice, mIndexBufferMemory, nullptr);
	}

	void Mesh::createVertexBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool)
	{
		//Size of buffer in bytes
		VkDeviceSize bufferSize = sizeof(Vertex) * mVertices.size();

		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		//Create buffer and allocate memory for it
		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO VERTEX BUFFER
		void* data;		//1. Create pointer to a point in normal memory
		//Map the vertex buffer memory to that point
		vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		//Copy memory from vertices vector to the point
		memcpy(data, mVertices.data(), (size_t)bufferSize);
		//Unamp vertex buffer memory
		vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);

		//Create destination vertex buffer for GPU memory
		createBuffer(mainDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mVertexBuffer, &mVertexBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer,
			mVertexBuffer, bufferSize);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
	}

	void Mesh::createIndexBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool)
	{
		//Get size of buffer for indices
		VkDeviceSize bufferSize = sizeof(uint32_t) * mIndices.size();
		
		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(mainDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO INDEX BUFFER
		void* data;		//1. Create pointer to a point in normal memory
		//Map the buffer memory to that point
		vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		//Copy memory from indices vector to the point
		memcpy(data, mIndices.data(), (size_t)bufferSize);
		//Unamp buffer memory
		vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);

		//Create destination vertex buffer for GPU memory
		createBuffer(mainDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mIndexBuffer, &mIndexBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer,
			mIndexBuffer, bufferSize);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
	}
}