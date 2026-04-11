#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
    class IVulkanSurface
    {
    public:
        virtual ~IVulkanSurface() = default;

        virtual vk::SurfaceKHR handle() const = 0;
    };
}