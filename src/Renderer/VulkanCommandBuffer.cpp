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

    void VulkanCommandBuffer::flush(VkDevice device, VkQueue queue, const VkFence fence, const std::vector<VkSemaphore>& signalSemaphores) const
	{
		if(mCommandBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		VK_CHECK(vkEndCommandBuffer(mCommandBuffer));

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &mCommandBuffer;
		if(!signalSemaphores.empty())
		{
			submit_info.pSignalSemaphores = signalSemaphores.data();
			submit_info.signalSemaphoreCount = signalSemaphores.size();
		}

		// Submit to the queue
		VkResult result = vkQueueSubmit(queue, 1, &submit_info, fence);
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, MAX(uint64_t)));
	}

	void VulkanCommandBuffer::free(VkDevice device, VkCommandPool commandPool, const bool cleanup)
	{
		if(commandPool && cleanup)
		{
			vkFreeCommandBuffers(device, commandPool, 1, &mCommandBuffer);
		}
	}
}