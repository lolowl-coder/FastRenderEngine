#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanCommandBuffer
    {
        void create(VkCommandPool graphicsCommandPool, VkDevice logicalDevice);
        void begin();
        void recordCommands();
        void end();

        VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    };
}