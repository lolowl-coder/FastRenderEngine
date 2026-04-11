#include "fre/core/Log.hpp"
#include "fre/platform/IGLFWWindow.hpp"
#include "fre/renderer/backend/vulkan/VulkanContext.hpp"
#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanRenderBackend.hpp"
#include "fre/renderer/backend/vulkan/VulkanSurfaceWindows.hpp"

namespace fre
{
	VulkanContext::VulkanContext(const Desc& desc)
		: mDesc(desc)
	{
		createInstance();
	}

	bool VulkanContext::createInstance()
	{
		if(volkInitialize() != VK_SUCCESS)
			return false;

		vk::detail::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr getProc =
			dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

		VULKAN_HPP_DEFAULT_DISPATCHER.init(getProc);

		std::vector<const char*> layers;
		if(mDesc.mEnableValidation)
			layers.push_back("VK_LAYER_KHRONOS_validation");

		// Application
		vk::ApplicationInfo appInfo{};
		appInfo.pApplicationName = "FRE";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		LOG_INFO("Vulkan API version: {}.{}.{}",
			VK_VERSION_MAJOR(appInfo.apiVersion),
			VK_VERSION_MINOR(appInfo.apiVersion),
			VK_VERSION_PATCH(appInfo.apiVersion));

		// Create instance
		vk::InstanceCreateInfo ici{};
		ici.pApplicationInfo = &appInfo;
		ici.enabledLayerCount = static_cast<uint32_t>(layers.size());
		ici.ppEnabledLayerNames = layers.data();

		// Enable extensions
		std::vector<const char*> enabledExtensions;
		for(auto& ext : mDesc.mInstanceExtensions)
		{
			if(ext.enabled)
			{
				enabledExtensions.push_back(ext.name.c_str());
			}
		}
		ici.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		ici.ppEnabledExtensionNames = enabledExtensions.data();

		mInstance = vkCheck(vk::createInstance(ici));

		VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance);

		volkLoadInstance(mInstance);

		return true;
	}

	SurfacePtr VulkanContext::createSurface(IWindow* window)
	{
		auto glfwWindow = dynamic_cast<IGLFWWindow*>(window);
		assert(glfwWindow != nullptr && "window must be IGLFWindow");

		return std::make_unique<VulkanSurfaceWindows>(mInstance, glfwWindow);
	}
}