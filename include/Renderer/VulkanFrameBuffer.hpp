#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/VulkanAttachment.hpp"

namespace fre
{
    struct MainDevice;

    struct VulkanFrameBuffer
    {
        void create(const MainDevice& mainDevice, VkImageView swapChainImageView,
            VkExtent2D swapChainExtent, VkRenderPass renderPass);
        void destroy(VkDevice logicalDevice);

    private:
        void createFrameBuffers(VkDevice logicalDevice, VkImageView swapChainImageView,
            VkExtent2D swapChainExtent, VkRenderPass renderPass);

    public:
        VulkanAttachment mColorAttachment;
        VulkanAttachment mDepthAttachment;
        VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;
    };
}