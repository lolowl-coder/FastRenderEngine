#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanCommandBuffer.hpp"
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

		//MAP MEMORY TO BUFFER
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

	bool VulkanBufferManager::isBufferAvailable(uint32_t index) const
	{
		return index < mBuffers.size();
	}

	const VulkanBuffer& VulkanBufferManager::getBuffer(uint32_t index) const
	{
		return mBuffers[index];
	}

    void createBuffer(const MainDevice& mainDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
	{
		//Information to create a buffer (doesn't include assigning memory)
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		//Multiple types of buffers possible
		bufferInfo.usage = bufferUsage;
		//Similar to swap chain images can share vertex buffers
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(mainDevice.logicalDevice, &bufferInfo, nullptr, buffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Vertex Buffer!");
		}

		//Get buffer memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mainDevice.logicalDevice, *buffer, &memRequirements);

		//ALLOCATE MEMORY TO BUFFER
		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memRequirements.size;
		//index of memory type on Physical Device that has required bit flags
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memRequirements.memoryTypeBits,
			bufferProperties);
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - CPU can interact with memory
		//VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

		//Allocate memory to VkDeviceMemory
		result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, bufferMemory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
		}

		//Allocate memory for given buffer
		vkBindBufferMemory(mainDevice.logicalDevice, *buffer, *bufferMemory, 0);
	}

	void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
	{
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

		//Region of data to copy from and to
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size = bufferSize;

		//Command to copy srcBuffer to dstBuffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

		endAndSubmitCommitBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
	}
}