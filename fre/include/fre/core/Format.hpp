#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
    enum class Format
    {
        Undefined,

        R8_UNorm,
        RG8_UNorm,
        RGBA8_UNorm,

        BGRA8_UNorm,

        D16,
        D24S8,
        D32
    };

    vk::Format toVk(fre::Format format);
}