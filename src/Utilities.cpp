#include "Utilities.hpp"

namespace fre
{
    std::vector<char> readFile(const std::string& fileName)
	{
		//Open stream from given file
		std::ifstream file(fileName, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file! " + fileName);
		}

		const auto fileSize = (size_t)file.tellg();
		file.seekg(0);
		std::vector<char> fileBuffer(fileSize);
		file.read(fileBuffer.data(), fileSize);

		file.close();

		return fileBuffer;
	}

    uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
	{
		//Get properties of physical device memory
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((allowedTypes & (i << 1))	//Index of memory type must match corresponding bit in allowedTypes
				&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)	//Desired property bit flags are part of memory type's property flags
			{
				return i;
			}
		}

		return std::numeric_limits<uint32_t>::max();
	}

    void createBuffer(const MainDevice& mainDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
	{
		//Information to create a buffer (doesn't include assigning memory)
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsage;	//Multiple types of buffers possible
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		//Similar to swap chain images can share vertex buffers

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

		//Allocate memory for given vertex buffer
		vkBindBufferMemory(mainDevice.logicalDevice, *buffer, *bufferMemory, 0);
	}

    VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
	{
		//Command buffer to hold transfer command
		VkCommandBuffer commandBuffer;

		//Command Buffer details
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		//Allocate command buffer from pool
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		//Information to begin the command buffer record
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	//We're only using the command buffer once, so set up for one time submit

		//Begin recording transfer commands
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

    void endAndSubmitCommitBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//Submit transfer commands to transfer queue and wait until it finished
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		//Free temporary command buffer back to pool
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
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

	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		//Get all Queue Family properties info for the given device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

		//Go through each queue family and check if it has at least 1 of the required types of queue
		int i = 0;
		for (const auto& queueFamily : queueFamilyList)
		{
			//First check if queue family has at least 1 queue in that family (could have no queues)
			//Queue can be of multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if it has required type
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i; //If queue family invalid get index
			}

			//Check if queue family supports presentation
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
			//Check if queue is presentation type (can be both graphics and presentation)
			if (queueFamily.queueCount > 0 && presentationSupport)
			{
				indices.presentationFamily = i;
			}

			//Check if queue family indices are in a valid state, stop searching if so
			if (indices.isValid())
			{
				break;
			}
			i++;
		}

		return indices;
	}
}