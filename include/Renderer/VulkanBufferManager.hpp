#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;
    
    struct VulkanBuffer
    {
        VkBuffer mBuffer;
		VkDeviceMemory mBufferMemory;
    };

    struct VulkanBufferManager
    {
        void destroy(VkDevice logicalDevice);
        
        void createBuffer(const MainDevice& mainDevice, VkQueue transferQueue,
            VkCommandPool transferCommandPool, VkBufferUsageFlagBits bufferUsage,
            const void* data, size_t size);

        bool isBufferAvailable(uint32_t index) const;
        const VulkanBuffer& getBuffer(uint32_t index) const;

        std::vector<VulkanBuffer> mBuffers;
    };
}