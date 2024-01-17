#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace fre
{
    struct MainDevice;

    enum EAttachmentKind
    {
        AK_COLOR,
        AK_DEPTH,
        AK_COUNT
    };

    struct VulkanAttachment
    {
        void create(const MainDevice& mainDevice, EAttachmentKind attachmentKind, VkExtent2D swapChainExtent);
        void destroy(VkDevice logicalDevice);

        VkImageView mImageView = VK_NULL_HANDLE;
    private:
        VkImage mImage = VK_NULL_HANDLE;
        VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
    };
}