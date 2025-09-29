#include "Renderer/VulkanRenderer.hpp"
#include "CudaUtilities.hpp"
#include "Interop/Interop.hpp"

namespace fre
{
	void importCudaExternalMemory(void** cudaPtr, cudaExternalMemory_t& cudaMem,
		VkDeviceMemory& vkMem, VkDeviceSize size,
		VkExternalMemoryHandleTypeFlagBits handleType, VulkanRenderer* renderer)
	{
		cudaExternalMemoryHandleDesc externalMemoryHandleDesc = {};

		if(handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
		{
			externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
		}
		else if(handleType &
			VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT)
		{
			externalMemoryHandleDesc.type =
				cudaExternalMemoryHandleTypeOpaqueWin32Kmt;
		}
		else if(handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT)
		{
			externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
		}
		else
		{
			throw std::runtime_error("Unknown handle type requested!");
		}

		externalMemoryHandleDesc.size = size;

#ifdef _WIN64
		externalMemoryHandleDesc.handle.win32.handle =
			(HANDLE)renderer->getMemHandle(vkMem, handleType);
#else
		externalMemoryHandleDesc.handle.fd =
			(int)(uintptr_t)getMemHandle(vkMem, handleType);
#endif

		CUDA_CHECK(cudaImportExternalMemory(&cudaMem, &externalMemoryHandleDesc));

		cudaExternalMemoryBufferDesc externalMemBufferDesc = {};
		externalMemBufferDesc.offset = 0;
		externalMemBufferDesc.size = size;
		externalMemBufferDesc.flags = 0;

		CUDA_CHECK(cudaExternalMemoryGetMappedBuffer(cudaPtr, cudaMem, &externalMemBufferDesc));
	}

	void importCudaExternalSemaphore(
		cudaExternalSemaphore_t& cudaSem, VkSemaphore& vkSem,
		VkExternalSemaphoreHandleTypeFlagBits handleType, VulkanRenderer* renderer)
	{
		cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc = {};

		if(handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT)
		{
			externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
		}
		else if(handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT)
		{
			externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32Kmt;
		}
		else if(handleType & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT)
		{
			externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
		}

		externalSemaphoreHandleDesc.handle.win32.handle =
			(HANDLE)renderer->getSemaphoreHandle(vkSem, handleType);

		externalSemaphoreHandleDesc.flags = 0;

		CUDA_CHECK(cudaImportExternalSemaphore(&cudaSem, &externalSemaphoreHandleDesc));
	}
}