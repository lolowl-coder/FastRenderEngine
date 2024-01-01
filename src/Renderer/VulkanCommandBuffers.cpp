#include "Renderer/VulkanCommandBuffers.hpp"

#include <stdexcept>
#include <array>

namespace fre
{
	void VulkanCommandBuffers::create(
		uint32_t count, VkCommandPool graphicsCommandPool, VkDevice logicalDevice)
	{
		mCommandBuffers.resize(count);

		VkCommandBufferAllocateInfo cbAllocInfo = {};
		cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbAllocInfo.commandPool = graphicsCommandPool;
		cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	//VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer you submit directly to queue. Can't be called by other buffers.
																//VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		cbAllocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &cbAllocInfo, mCommandBuffers.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Command Buffers!");
		}
	}

    void VulkanCommandBuffers::begin(uint32_t currentImageIndex)
    {
        //Information about how to begin each command buffer
		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//Start recording commands to command buffer
		VkResult result = vkBeginCommandBuffer(get(currentImageIndex), &bufferBeginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}
    }

    void VulkanCommandBuffers::end(uint32_t currentImageIndex)
    {
		//Stop recording
		VkResult result = vkEndCommandBuffer(mCommandBuffers[currentImageIndex]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to stop recording a Command Buffer!");
		}
    }

    VkCommandBuffer VulkanCommandBuffers::get(uint32_t currentImageIndex)
    {
        return mCommandBuffers[currentImageIndex];
    }
}