#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace fre
{
    struct MainDevice;

    struct VulkanRenderPass
    {
        void create(const MainDevice& mainDevice, VkFormat swapChainImageFormat);
        void begin(
            VkFramebuffer swapChainFrameBuffer,
            VkExtent2D swapChainExtent, VkCommandBuffer commandBuffer);
        void end(VkCommandBuffer commandBuffer);
        void destroy(VkDevice logicalDevice);

        VkRenderPass mRenderPass;
    };
    
}