#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Types.hpp"

#include <vector>

namespace fre
{
    struct VulkanBuffer
    {
        VkBuffer mBuffer = VK_NULL_HANDLE;
		VkDeviceMemory mBufferMemory = VK_NULL_HANDLE;
        uint64_t mDeviceAddress = 0;
		size_t mSize = 0;

        bool isValid() const
        {
            return mBuffer != VK_NULL_HANDLE;
		}

        bool operator ==(const VulkanBuffer& other) const
        {
            return mBuffer == other.mBuffer;
        }
    };

    class VulkanBufferManager
    {
    public:
		VulkanBufferManager(const MainDevice& mainDevice)
            : mMainDevice(mainDevice)
        {
        }
        void destroy();
        
        void destroyBuffer(VulkanBuffer& buffer);

        VulkanBuffer createStagingBuffer(VkQueue transferQueue,
		    VkCommandPool transferCommandPool, const void* data, size_t size);

        uint32_t createBuffer(VkQueue transferQueue,
            VkCommandPool transferCommandPool, VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryFlags, const void* data, const size_t bufSize);

        uint32_t createBuffer(VkQueue transferQueue,
            VkCommandPool transferCommandPool, VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryFlags, const void* data, const size_t bufSize, const size_t dataSize);
        const VulkanBuffer& createExternalBuffer(VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryFlags, VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size);

        uint8_t* map(uint32_t index, size_t size);
        void unmap(const uint32_t index);

        void udpateBuffer(uint32_t index, const void* data, size_t size);

        bool isBufferAvailable(uint32_t index) const;
        VulkanBuffer getBuffer(uint32_t index) const;

    private:
        std::vector<VulkanBuffer> mBuffers;
        MainDevice mMainDevice;
    };
}