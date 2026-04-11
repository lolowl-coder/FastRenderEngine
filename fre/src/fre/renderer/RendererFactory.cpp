#include "fre/core/IWindow.hpp"
#include "fre/renderer/RendererFactory.hpp"
#include "fre/renderer/Renderer.hpp"
#include "fre/renderer/backend/vulkan/VulkanContext.hpp"
#include "fre/renderer/backend/vulkan/VulkanRenderBackend.hpp"
#include "fre/platform/IGLFWWindow.hpp"

namespace fre
{
    GraphicsContextPtr createGraphicsContext(const Backend backend, IWindow* window, const bool enableValidation)
    {
        switch (backend)
        {
            case Backend::Vulkan:
            {
				auto glfwWindow = dynamic_cast<IGLFWWindow*>(window);
				// To render to a window surface, we need to enable the required instance extensions.
                // We can get these from the window itself, as it knows which extensions it needs.
				auto glfwInstanceExtensions = glfwWindow->getInstanceExtensions();
                VulkanContext::Desc desc =
                {
                    .mEnableValidation = enableValidation
                };
				// Merge the required instance extensions from the window with the default ones in the VulkanContext::Desc
				desc.mInstanceExtensions.insert(desc.mInstanceExtensions.end(), glfwInstanceExtensions.begin(), glfwInstanceExtensions.end());
                return std::make_unique<VulkanContext>(desc);
            }
        }
        return nullptr;
	}

    RendererPtr createRenderer(const CommonRendererConfig& commonConfig)
    {
        switch (commonConfig.mBackend)
        {
            case Backend::Vulkan:
                return std::make_unique<Renderer>(commonConfig);
        }
        return nullptr;
    }
}