#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <cuda_runtime_api.h>

namespace fre
{
	class VulkanRenderer;

	void importCudaExternalMemory(void** cudaPtr, cudaExternalMemory_t& cudaMem,
		VkDeviceMemory& vkMem, VkDeviceSize size,
		VkExternalMemoryHandleTypeFlagBits handleType, fre::VulkanRenderer* renderer);

	void importCudaExternalSemaphore(
		cudaExternalSemaphore_t& cudaSem, VkSemaphore& vkSem,
		VkExternalSemaphoreHandleTypeFlagBits handleType, fre::VulkanRenderer* renderer);
}