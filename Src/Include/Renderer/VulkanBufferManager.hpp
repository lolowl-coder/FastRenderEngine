#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;
    
    struct VulkanBuffer
    {
        VkBuffer mBuffer = VK_NULL_HANDLE;
		VkDeviceMemory mBufferMemory = VK_NULL_HANDLE;
        uint64_t mDeviceAddress = 0;

        bool operator ==(const VulkanBuffer& other) const
        {
            return mBuffer == other.mBuffer;
        }
    };

    struct VulkanBufferManager
    {
        void destroy(VkDevice logicalDevice);
        
        void destroyBuffer(VkDevice logicalDevice, VulkanBuffer& buffer);

        VulkanBuffer createStagingBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		    VkCommandPool transferCommandPool, const void* data, size_t size);
        const VulkanBuffer& createBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
            VkCommandPool transferCommandPool, VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryFlags, const void* data, size_t size);
        const VulkanBuffer& createExternalBuffer(const MainDevice& mainDevice, VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryFlags, VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size);

        bool isBufferAvailable(uint32_t index) const;
        const VulkanBuffer* getBuffer(uint32_t index) const;

        std::vector<VulkanBuffer> mBuffers;
    };
}