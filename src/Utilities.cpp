#include "Utilities.hpp"

namespace fre
{
    std::vector<char> readFile(const std::string& fileName)
	{
		//Open stream from given file
		std::ifstream file(fileName, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}

		const auto fileSize = (size_t)file.tellg();
		file.seekg(0);
		std::vector<char> fileBuffer(fileSize);
		file.read(fileBuffer.data(), fileSize);

		file.close();

		return fileBuffer;
	}

	VkFormat chooseSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		//Loop through options an find compatible one
		for (VkFormat format : formats)
		{
			//Get properties for given format on this device
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			//Depending on tiling choise need to check for different bit flag
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
		}
		
		throw std::runtime_error("Failed to find a matching format!");
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

    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
	{
		//Information to create a buffer (doesn't include assigning memory)
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsage;	//Multiple types of buffers possible
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		//Similar to swap chain images can share vertex buffers

		VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Vertex Buffer!");
		}

		//Get buffer memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

		//ALLOCATE MEMORY TO BUFFER
		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memRequirements.size;
		//index of memory type on Physical Device that has required bit flags
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,
			bufferProperties);
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - CPU can interact with memory
		//VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

		//Allocate memory to VkDeviceMemory
		result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
		}

		//Allocate memory for given vertex buffer
		vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
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

	void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
	{
		//Create buffer
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

		VkBufferImageCopy imageRegion = {};
		imageRegion.bufferOffset = 0;	//Offset into data
		imageRegion.bufferRowLength = 0;	//Row length of data to calculate data spacing
		imageRegion.bufferImageHeight = 0;	//Image height to calculate data spacing
		imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	//Which aspect of image to copy
		imageRegion.imageSubresource.mipLevel = 0;	//Mip level to copy
		imageRegion.imageSubresource.baseArrayLayer = 0;	//Starting array layer (if presented)
		imageRegion.imageSubresource.layerCount = 1;	//Number of layers to copy starting at baseArrayLayer
		imageRegion.imageOffset = { 0, 0, 0 };	//Start origin (xyz)
		imageRegion.imageExtent = { width, height, 1 };	//Region size to copy (xyz)

		//Copy buffer to image
		vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

		endAndSubmitCommitBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
	}

	void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		//Create buffer
		VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = oldLayout;	//Lyaout to transition from
		imageMemoryBarrier.newLayout = newLayout;	//Lyaout to transition to
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	//Family to transition from
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	//Family to transition to
		imageMemoryBarrier.image = image;	//Image being accessed and modified as part of barrier
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;	//Mip level to start alternation on
		imageMemoryBarrier.subresourceRange.levelCount = 1;	//First mip level to start alternation on
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;	//First layer to start alternation on
		imageMemoryBarrier.subresourceRange.layerCount = 1;	//Number of layers to start alternation on

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;	//Memory access stage transition must happen after ...
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	//Memory access stage transition must happen before ...

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		//from transfer destination to shader readable
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		if(srcStage != VK_PIPELINE_STAGE_NONE && dstStage != VK_PIPELINE_STAGE_NONE)
		{
			vkCmdPipelineBarrier(commandBuffer,
				srcStage, dstStage,	//Pipeline stages (match to src and dst AccessMasks)
				0,		//Dependency flags
				0, nullptr,	//Memory Barrier count + data
				0, nullptr,	//Buffer Memory Barrier count + data
				1, &imageMemoryBarrier	//Buffer Image Memory Barrier count + data
				);
		}

		endAndSubmitCommitBuffer(device, commandPool, queue, commandBuffer);
	}
}