#include "Renderer/VulkanBufferManager.hpp"
#include "Log.hpp"
#include "Utilities.hpp"

#ifdef _WIN64
	#include <VersionHelpers.h>
	#include <dxgi1_2.h>
	#include <aclapi.h>
	#include <windows.h>
#endif /* _WIN64 */

#include <exception>

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

	VulkanBuffer VulkanBufferManager::createStagingBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, const void* data, size_t size)
	{
		VulkanBuffer result;

		fre::createBuffer(mainDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&result.mBuffer, &result.mBufferMemory);

		//MAP MEMORY TO BUFFER
		void* mappedData;		//1. Create pointer to a point in normal memory
		//Map the vertex buffer memory to that point
		VK_CHECK(vkMapMemory(mainDevice.logicalDevice, result.mBufferMemory, 0, size, 0, &mappedData));
		//Copy memory from vertices vector to the point
		memcpy(mappedData, data, size);
		//Unamp vertex buffer memory
		vkUnmapMemory(mainDevice.logicalDevice, result.mBufferMemory);

		mBuffers.push_back(result);

		return result;
	}
    
    //Data size in bytes. Example: sizeof(Vertex) * mVertices.size();
    const VulkanBuffer& VulkanBufferManager::createBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		VkCommandPool transferCommandPool, VkBufferUsageFlagBits bufferUsage,
		VkMemoryPropertyFlags memoryFlags, const void* data, size_t size)
	{
		//Temporary buffer to "stage" vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		try
		{
			//Create buffer and allocate memory for it
			fre::createBuffer(mainDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer, &stagingBufferMemory);

			//MAP MEMORY TO BUFFER
			void* mappedData;		//1. Create pointer to a point in normal memory
			//Map the vertex buffer memory to that point
			VK_CHECK(vkMapMemory(mainDevice.logicalDevice, stagingBufferMemory, 0, size, 0, &mappedData));
			//Copy memory from vertices vector to the point
			memcpy(mappedData, data, size);
			//Unamp vertex buffer memory
			vkUnmapMemory(mainDevice.logicalDevice, stagingBufferMemory);
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
		}
		catch(...)
		{
			LOG_ERROR("Unknown exception");
		}

		mBuffers.push_back(VulkanBuffer());
		auto& buffer = mBuffers.back();
		//Create destination vertex buffer for GPU memory
		fre::createBuffer(mainDevice, size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage,
			memoryFlags, &buffer.mBuffer, &buffer.mBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer,
			buffer.mBuffer, size);

		//Clean up staging buffer parts
		vkDestroyBuffer(mainDevice.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, stagingBufferMemory, nullptr);

		return buffer;
	}

	const VulkanBuffer& VulkanBufferManager::createExternalBuffer(
		const MainDevice& mainDevice, VkBufferUsageFlagBits bufferUsage,
            VkMemoryPropertyFlags memoryFlags, VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkExternalMemoryBufferCreateInfo externalMemoryBufferInfo = {};
		externalMemoryBufferInfo.sType =
			VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
		externalMemoryBufferInfo.handleTypes = extMemHandleType;
		bufferInfo.pNext = &externalMemoryBufferInfo;

		mBuffers.push_back(VulkanBuffer());
		auto& buffer = mBuffers.back();

		if(vkCreateBuffer(mainDevice.logicalDevice, &bufferInfo, nullptr, &buffer.mBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mainDevice.logicalDevice, buffer.mBuffer, &memRequirements);

		#ifdef _WIN64
			WindowsSecurityAttributes winSecurityAttributes;

			VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
			vulkanExportMemoryWin32HandleInfoKHR.sType =
				VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
			vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
			vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
			vulkanExportMemoryWin32HandleInfoKHR.dwAccess =
				DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
			vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;
		#endif /* _WIN64 */
		VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
		vulkanExportMemoryAllocateInfoKHR.sType =
			VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
		#ifdef _WIN64
			vulkanExportMemoryAllocateInfoKHR.pNext =
				extMemHandleType & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
				? &vulkanExportMemoryWin32HandleInfoKHR
				: NULL;
			vulkanExportMemoryAllocateInfoKHR.handleTypes = extMemHandleType;
		#else
			vulkanExportMemoryAllocateInfoKHR.pNext = NULL;
			vulkanExportMemoryAllocateInfoKHR.handleTypes =
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
		#endif /* _WIN64 */
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = &vulkanExportMemoryAllocateInfoKHR;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryTypeIndex(
			mainDevice.physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

		VK_CHECK(vkAllocateMemory(mainDevice.logicalDevice, &allocInfo, nullptr, &buffer.mBufferMemory));

		vkBindBufferMemory(mainDevice.logicalDevice, buffer.mBuffer, buffer.mBufferMemory, 0);

		return buffer;
	}

	bool VulkanBufferManager::isBufferAvailable(uint32_t index) const
	{
		return index < mBuffers.size();
	}

	const VulkanBuffer* VulkanBufferManager::getBuffer(uint32_t index) const
	{
		const VulkanBuffer* result = isBufferAvailable(index) ? &mBuffers[index] : nullptr;

		return result;
	}
}