#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

namespace fre
{
    struct VulkanSamplerKey
    {
        VkSamplerAddressMode mAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkFilter mFilter = VK_FILTER_LINEAR;
        uint32_t mUnnormalizedCoordinates = VK_FALSE;

        bool operator==(const VulkanSamplerKey& other) const
        {
            return
                mAddressMode == other.mAddressMode &&
                mFilter == other.mFilter &&
                mUnnormalizedCoordinates == other.mUnnormalizedCoordinates;
        }
    };
}