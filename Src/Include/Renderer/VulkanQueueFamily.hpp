#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanQueueFamily
    {
        void init(VkPhysicalDevice device, VkSurfaceKHR surface,
            const VkQueueFamilyProperties &queueFamilyProperties, uint8_t familyIndex);

        bool isFullySupported() const;

        bool mHasGraphicsSupport = false;
        bool mHasPresentationSupport = false;
        bool mHasTransferSupport = false;
        bool mHasComputeSupport = false;
        uint32_t mQueueCount = 0;
        int8_t mId = -1;
    };
}