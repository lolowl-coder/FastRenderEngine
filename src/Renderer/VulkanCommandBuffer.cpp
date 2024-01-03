#include "Renderer/VulkanCommandBuffer.hpp"

#include <stdexcept>
#include <array>

namespace fre
{
	void VulkanCommandBuffer::create(
		VkCommandPool graphicsCommandPool, VkDevice logicalDevice)
	{
		VkCommandBufferAllocateInfo cbAllocInfo = {};
		cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbAllocInfo.commandPool = graphicsCommandPool;
		cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	//VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer you submit directly to queue. Can't be called by other buffers.
																//VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		cbAllocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &cbAllocInfo, &mCommandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Command Buffers!");
		}
	}

    void VulkanCommandBuffer::begin()
    {
        //Information about how to begin each command buffer
		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//Start recording commands to command buffer
		VkResult result = vkBeginCommandBuffer(mCommandBuffer, &bufferBeginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}

    }

    void VulkanCommandBuffer::end()
    {
		//Stop recording
		VkResult result = vkEndCommandBuffer(mCommandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to stop recording a Command Buffer!");
		}
    }
}