#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/VulkanAttachment.hpp"

#include <vector>

namespace fre
{
    struct MainDevice;

    struct VulkanFrameBuffer
    {
        void create(const MainDevice& mainDevice, std::vector<VkImageView> attachmentsViews,
            VkExtent2D swapChainExtent, VkRenderPass renderPass);
        void addColorAttachment(const VulkanAttachment& colorAttachment);
        void setDepthAttachment(const VulkanAttachment& depthAttachment);
        void destroy(VkDevice logicalDevice);

        std::vector<VulkanAttachment> mColorAttachments;
        VulkanAttachment mDepthAttachment;
        VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;
    };
}