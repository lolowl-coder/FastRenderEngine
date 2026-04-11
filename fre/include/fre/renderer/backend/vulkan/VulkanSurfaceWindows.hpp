#pragma once

#include <windows.h>

#include "fre/renderer/ISurface.hpp"
#include "fre/renderer/backend/vulkan/Frame.hpp"
#include "fre/renderer/backend/vulkan/IVulkanSurface.hpp"
#include "fre/core/Pointers.hpp"

namespace fre
{
    class IGLFWWindow;

    class VulkanSurfaceWindows : public ISurface, public IVulkanSurface
    {
    public:
        VulkanSurfaceWindows(vk::Instance vulkanInstance, const IGLFWWindow* window);
        ~VulkanSurfaceWindows();

        virtual vk::SurfaceKHR handle() const override { return mSurface; }

        virtual uint32_t width() const override;
        virtual uint32_t height() const override;
    private:
        const IGLFWWindow* mWindow;
        const vk::Instance mInstance = VK_NULL_HANDLE;
        vk::SurfaceKHR mSurface = VK_NULL_HANDLE;
    };
}