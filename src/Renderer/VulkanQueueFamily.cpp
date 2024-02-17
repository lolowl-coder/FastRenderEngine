#include "Renderer/VulkanQueueFamily.hpp"

namespace fre
{
    void VulkanQueueFamily::init(VkPhysicalDevice device, VkSurfaceKHR surface,
        const VkQueueFamilyProperties &queueFamilyProperties, uint8_t familyIndex)
    {
        //We are looking for graphics, presentation and transfer support
        mHasGraphicsSupport = (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        //Check if queue family supports presentation
        VkBool32 hasPresentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, surface, &hasPresentationSupport);
        mHasPresentationSupport = hasPresentationSupport == VK_TRUE;
        mHasTransferSupport = (queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0;

        mQueueCount = queueFamilyProperties.queueCount;
        mId = familyIndex;
    }

    bool VulkanQueueFamily::isFullySupported() const
    {
        return mHasGraphicsSupport && mHasPresentationSupport && mHasTransferSupport;
    }
}