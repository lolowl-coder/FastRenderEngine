#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;

    enum class EAttachmentKind
    {
        Color,
        Color16,
        Color32,
        DepthStencil,
        Count
    };

    std::vector<VkFormat> getImageFormats(EAttachmentKind attachmentKind);

    struct VulkanAttachment
    {
        void create(const MainDevice& mainDevice, EAttachmentKind attachmentKind, VkExtent2D swapChainExtent);
        void destroy(VkDevice logicalDevice);

        VkImageView mImageView = VK_NULL_HANDLE;
        VkImage mImage = VK_NULL_HANDLE;
    private:
        VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
    };
}