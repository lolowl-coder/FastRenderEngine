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
    };

    struct VulkanBufferManager
    {
        void destroy(VkDevice logicalDevice);
        
        VulkanBuffer createStagingBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
		    VkCommandPool transferCommandPool, const void* data, size_t size);
        const VulkanBuffer& createBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
            VkCommandPool transferCommandPool, VkBufferUsageFlagBits bufferUsage,
            VkMemoryPropertyFlags memoryFlags, const void* data, size_t size);
        const VulkanBuffer& createExternalBuffer(const MainDevice& mainDevice, VkBufferUsageFlagBits bufferUsage,
            VkMemoryPropertyFlags memoryFlags, VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size);

        bool isBufferAvailable(uint32_t index) const;
        const VulkanBuffer* getBuffer(uint32_t index) const;

        std::vector<VulkanBuffer> mBuffers;
    };
}