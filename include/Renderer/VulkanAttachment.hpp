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

    class VulkanAttachment
    {
    public:
        void create(const MainDevice& mainDevice, EAttachmentKind attachmentKind, VkExtent2D swapChainExtent);
        void destroy(VkDevice logicalDevice);

    public:
        VkImage mImage = VK_NULL_HANDLE;
        VkImageView mImageView = VK_NULL_HANDLE;
    private:
        VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
    };
}