#include "fre/core/PlatformFactory.hpp"
#include "fre/platform/FileSystemWindows.hpp"
#include "fre/platform/WindowManagerWindows.hpp"
#include "fre/renderer/backend/vulkan/VulkanSurfaceWindows.hpp"
#include "fre/renderer/backend/vulkan/VulkanImage.hpp"
#include "fre/renderer/backend/vulkan/VulkanImageView.hpp"

#include <memory>

namespace fre
{
    FileSystemPtr createFileSystem()
    {
        return std::make_unique<FileSystemWindows>();
    }

    WindowManagerPtr createWindowManager()
    {
        return std::make_unique<WindowManagerWindows>();
	}
}