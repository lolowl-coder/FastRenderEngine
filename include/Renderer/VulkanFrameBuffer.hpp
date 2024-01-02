#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace fre
{
    struct MainDevice;

    struct VulkanFrameBuffer
    {
        void create(const MainDevice& mainDevice, VkImageView swapChainImageView,
            VkExtent2D swapchainExtent, VkRenderPass renderPass);
        void destroy(VkDevice logicalDevice);

    private:
        void createColourBufferImage(const MainDevice& mainDevice,
            VkExtent2D swapChainExtent);
        void createDepthBufferImage(const MainDevice& mainDevice,
            VkExtent2D swapChainExtent);
        void createFrameBuffers(VkDevice logicalDevice, VkImageView swapChainImageView,
            VkExtent2D swapChainExtent, VkRenderPass renderPass);
            
    public:
		VkImage mColourBufferImage = VK_NULL_HANDLE;
		VkDeviceMemory mColourBufferImageMemory = VK_NULL_HANDLE;
		VkImageView mColourBufferImageView = VK_NULL_HANDLE;

        VkImage mDepthBufferImage = VK_NULL_HANDLE;
		VkDeviceMemory mDepthBufferImageMemory = VK_NULL_HANDLE;
		VkImageView mDepthBufferImageView = VK_NULL_HANDLE;

        VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;
    };
}