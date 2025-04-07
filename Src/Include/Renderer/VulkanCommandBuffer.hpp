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

        VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    };
}