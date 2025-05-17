#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanCommandBuffer
    {
        void allocate(VkCommandPool graphicsCommandPool, VkDevice logicalDevice);
        void begin() const;
        void end() const;
        void flush(VkDevice device, VkQueue queue, const VkFence fence, const std::vector<VkSemaphore>& signalSemaphores) const;
        void free(VkDevice device, VkCommandPool commandPool, const bool cleanup);


        VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    };
}