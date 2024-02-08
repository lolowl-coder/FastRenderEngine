#include "Renderer/VulkanBufferManager.hpp"
#include "Utilities.hpp"

namespace fre
{
    void VulkanBufferManager::destroy(VkDevice logicalDevice)
    {
        for(auto& buffer : mBuffers)
        {
            vkDestroyBuffer(logicalDevice, buffer.mBuffer, nullptr);
            vkFreeMemory(logicalDevice, buffer.mBufferMemory, nullptr);
        }
    }
    
    //Data size in bytes. Example: sizeof(Vertex) * mVertices.size();
    void VulkanBufferManager::createBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, VkBufferUsageFlagBits bufferUsage,
		const void* data, size_t size)
	{
		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		//Create buffer and allocate memory for it
		fre::createBuffer(mainDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO VERTEX BUFFER
		void* mappedData;		//1. Create pointer to a point in normal memory
		//Map the vertex buffer memory to that point
		vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, size, 0, &mappedData);
		//Copy memory from vertices vector to the point
		memcpy(mappedData, data, size);
		//Unamp vertex buffer memory
		vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);

		mBuffers.push_back(VulkanBuffer());
		auto& buffer = mBuffers.back();
		//Create destination vertex buffer for GPU memory
		fre::createBuffer(mainDevice, size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffer.mBuffer, &buffer.mBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer,
			buffer.mBuffer, size);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);
	}

	const VulkanBuffer& VulkanBufferManager::getBuffer(uint32_t index)
	{
		if (index >= mBuffers.size())
		{
			throw std::runtime_error("Attempted to access invalid Buffer index!");
		}

		return mBuffers[index];
	}
}