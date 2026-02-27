#include "Renderer/VulkanBufferManager.hpp"
#include "Log.hpp"
#include "Utilities.hpp"

#ifdef _WIN64
	#include <VersionHelpers.h>
	#include <dxgi1_2.h>
	#include <aclapi.h>
	#define NOMINMAX
	#include <windows.h>
#endif /* _WIN64 */

#include <exception>

namespace fre
{
    void VulkanBufferManager::destroy()
    {
		auto buffersToDestroy = mBuffers;
        for(auto& buffer : buffersToDestroy)
        {
			destroyBuffer(buffer);
        }
    }

    void VulkanBufferManager::destroyBuffer(VulkanBuffer& buffer)
    {
        auto& bufferIt = std::find(mBuffers.begin(), mBuffers.end(), buffer);
        if(bufferIt != mBuffers.end())
		{
			mBuffers.erase(bufferIt);
		}
		vkDestroyBuffer(mMainDevice.logicalDevice, buffer.mBuffer, nullptr);
		vkFreeMemory(mMainDevice.logicalDevice, buffer.mBufferMemory, nullptr);
        LOG_TRACE("Buffer destroyed: {}", (uint64_t)buffer.mBuffer);
    }

	VulkanBuffer VulkanBufferManager::createStagingBuffer(VkQueue transferQueue,
		VkCommandPool transferCommandPool, const void* data, size_t size)
	{
		VulkanBuffer result;

		fre::createBuffer(mMainDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0,
			&result.mBuffer, nullptr, &result.mBufferMemory);

		//MAP MEMORY TO BUFFER
		
		//Create pointer to a point in CPU memory
		void* mappedData;
		//Map the vertex buffer memory to that point
		VK_CHECK(vkMapMemory(mMainDevice.logicalDevice, result.mBufferMemory, 0, size, 0, &mappedData));
		//Copy memory from vertices vector to the point
		memcpy(mappedData, data, size);
		//Unamp vertex buffer memory
		vkUnmapMemory(mMainDevice.logicalDevice, result.mBufferMemory);

		mBuffers.push_back(result);

		return result;
	}
    
	uint32_t VulkanBufferManager::createBuffer(VkQueue transferQueue,
		VkCommandPool transferCommandPool, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags memoryFlags, const void* data, const size_t bufSize)
	{
		return createBuffer(transferQueue, transferCommandPool, bufferUsage, memoryFlags, data, bufSize, bufSize);
	}

    //Data size in bytes. Example: sizeof(Vertex) * mVertices.size();
    uint32_t VulkanBufferManager::createBuffer(VkQueue transferQueue,
		VkCommandPool transferCommandPool, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags memoryFlags, const void* data, const size_t bufSize, const size_t dataSize)
	{
		//Temporary buffer to "stage" vertex data before transferring to GPU
		VulkanBuffer stagingBuffer;

		try
		{
			//Create buffer and allocate memory for it
			fre::createBuffer(mMainDevice, bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				0,
				&stagingBuffer.mBuffer, nullptr, &stagingBuffer.mBufferMemory);

			//MAP MEMORY TO BUFFER
			//1. Create pointer in CPU-side memory
			void* mappedData;
			//Map the vertex buffer memory to that pointer
			VK_CHECK(vkMapMemory(mMainDevice.logicalDevice, stagingBuffer.mBufferMemory, 0, bufSize, 0, &mappedData));
			if(data != nullptr)
			{
				//Copy memory from vertices vector to the point
				memcpy(mappedData, data, dataSize);
			}
			else
			{
                memset(mappedData, 0, bufSize);
			}
			//Unamp buffer memory
			vkUnmapMemory(mMainDevice.logicalDevice, stagingBuffer.mBufferMemory);
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
		}
		catch(...)
		{
			LOG_ERROR("Unknown exception");
		}

        uint32_t result = mBuffers.size();
		mBuffers.push_back(VulkanBuffer());
		auto& buffer = mBuffers.back();
		//Create destination vertex buffer for GPU memory
		const bool deviceAddressRequested = (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0;
		fre::createBuffer(mMainDevice, bufSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage,
			memoryFlags, deviceAddressRequested ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR : 0,
			&buffer.mBuffer, deviceAddressRequested ? &buffer.mDeviceAddress : nullptr, &buffer.mBufferMemory);

		//Copy staging buffer to vertex buffer on GPU
		copyBuffer(mMainDevice.logicalDevice, transferQueue, transferCommandPool, stagingBuffer.mBuffer,
			buffer.mBuffer, dataSize);

		//Clean up staging buffer parts
		vkDestroyBuffer(mMainDevice.logicalDevice, stagingBuffer.mBuffer, nullptr);
		vkFreeMemory(mMainDevice.logicalDevice, stagingBuffer.mBufferMemory, nullptr);

		return result;
	}

	const VulkanBuffer& VulkanBufferManager::createExternalBuffer(
		VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryFlags,
		VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size)
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

		if(vkCreateBuffer(mMainDevice.logicalDevice, &bufferInfo, nullptr, &buffer.mBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}
		else
		{
            LOG_TRACE("External Vulkan buffer created: {}", (uint64_t)buffer.mBuffer);
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(mMainDevice.logicalDevice, buffer.mBuffer, &memRequirements);

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
			mMainDevice.physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

		VK_CHECK(vkAllocateMemory(mMainDevice.logicalDevice, &allocInfo, nullptr, &buffer.mBufferMemory));

		vkBindBufferMemory(mMainDevice.logicalDevice, buffer.mBuffer, buffer.mBufferMemory, 0);

		return buffer;
	}

	uint8_t* VulkanBufferManager::map(uint32_t index, size_t size)
	{
		void* result = nullptr;

		if(index < mBuffers.size())
		{
			VulkanBuffer& buffer = mBuffers[index];
			vkMapMemory(mMainDevice.logicalDevice, buffer.mBufferMemory, 0, size, 0, &result);
		}

        return static_cast<uint8_t*>(result);
	}

	void VulkanBufferManager::unmap(const uint32_t index)
	{
		if(index < mBuffers.size())
		{
			VulkanBuffer& buffer = mBuffers[index];
			vkUnmapMemory(mMainDevice.logicalDevice, buffer.mBufferMemory);
		}
	}

	void VulkanBufferManager::udpateBuffer(uint32_t index, const void* data, size_t size)
	{
		if(index < mBuffers.size())
		{
			VulkanBuffer& buffer = mBuffers[index];
			void* mappedData;
			vkMapMemory(mMainDevice.logicalDevice, buffer.mBufferMemory, 0, size, 0, &mappedData);
			memcpy(mappedData, data, size);
			vkUnmapMemory(mMainDevice.logicalDevice, buffer.mBufferMemory);
		}
	}


	bool VulkanBufferManager::isBufferAvailable(uint32_t index) const
	{
		return index < mBuffers.size();
	}

	VulkanBuffer VulkanBufferManager::getBuffer(uint32_t index) const
	{
		VulkanBuffer result;
		
		if(isBufferAvailable(index))
		{
			result = mBuffers[index];
		}

		return result;
	}
}