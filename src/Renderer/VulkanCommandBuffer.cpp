#include "Renderer/VulkanCommandBuffer.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <array>

namespace fre
{
	void VulkanCommandBuffer::allocate(
		VkCommandPool commandPool, VkDevice logicalDevice)
	{
		VkCommandBufferAllocateInfo cbAllocInfo = {};
		cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbAllocInfo.commandPool = commandPool;
		cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	//VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer you submit directly to queue. Can't be called by other buffers.
																//VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		cbAllocInfo.commandBufferCount = 1;

		VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &cbAllocInfo, &mCommandBuffer));
	}

    void VulkanCommandBuffer::begin() const
    {
        //Information about how to begin each command buffer
		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//Start recording commands to command buffer
		VK_CHECK(vkBeginCommandBuffer(mCommandBuffer, &bufferBeginInfo));
    }

    void VulkanCommandBuffer::end() const
    {
		//Stop recording
		VK_CHECK(vkEndCommandBuffer(mCommandBuffer));
    }
}