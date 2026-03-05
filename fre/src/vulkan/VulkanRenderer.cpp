#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Log.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include <volk.h>

#include <format>
#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace fre
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT              messageTypes,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		using namespace fre;

		if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		{
			LOG_ERROR(pCallbackData->pMessage);
		}
		else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			LOG_WARNING(pCallbackData->pMessage);
		}
		else
		{
			LOG_INFO(pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	VulkanRenderer::VulkanRenderer()
	{
		mFS = createFileSystem();
		mVFS = std::make_unique<VirtualFileSystem>(*mFS);

		Log::initialize(*mFS, true, true);
	}

	bool VulkanRenderer::selectQueueFamilies()
	{
		auto families = mPhysicalDevice.getQueueFamilyProperties();

		int graphicsIndex = -1;
		int computeIndex = -1;
		int transferIndex = -1;

		int dedicatedCompute = -1;
		int dedicatedTransfer = -1;

		for (uint32_t i = 0; i < families.size(); ++i)
		{
			const auto& props = families[i];

			bool graphics = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eGraphics);
			bool compute = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eCompute);
			bool transfer = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eTransfer);

			if (graphics && graphicsIndex == -1)
				graphicsIndex = i;

			// Dedicated compute: compute but not graphics
			if (compute && !graphics)
				dedicatedCompute = i;

			// Dedicated transfer: transfer but not graphics and not compute
			if (transfer && !graphics && !compute)
				dedicatedTransfer = i;

			if (compute && computeIndex == -1)
				computeIndex = i;

			if (transfer && transferIndex == -1)
				transferIndex = i;
		}

		if (graphicsIndex == -1)
			return false;

		mGraphicsQueueFamily = graphicsIndex;

		// Prefer dedicated compute
		if (dedicatedCompute != -1)
			mComputeQueueFamily = dedicatedCompute;
		else
			mComputeQueueFamily = computeIndex;

		// Prefer dedicated transfer
		if (dedicatedTransfer != -1)
			mTransferQueueFamily = dedicatedTransfer;
		else
			mTransferQueueFamily = transferIndex;

		return true;
	}

	bool VulkanRenderer::createDebugMessenger()
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo{};

		createInfo.messageSeverity =
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

		createInfo.messageType =
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		createInfo.pfnUserCallback = debugCallback;

		auto result = mInstance.createDebugUtilsMessengerEXT(createInfo);
		if (result.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create debug messenger");
		}
		mDebugMessenger = result.value;

		return true;
	}

	void evaluateFeature(bool supported, FeatureRequest& feature)
	{
		if (supported)
		{
			feature.enabled = true;
			LOG_INFO("Feature {} is supported and enabled", feature.name);
		}
		else
		{
			if (feature.requirement == FeatureRequirement::Required)
			{
				throw std::runtime_error(std::format("Feature {} not supported", feature.name));
			}
			else
			{
				LOG_WARNING("Feature {} not supported, but it's optional", feature.name);
				feature.enabled = false;
			}
		}
	}

	void VulkanRenderer::evaluateFeatures()
	{
		vk::PhysicalDeviceVulkan13Features features13{};
		vk::PhysicalDeviceVulkan12Features features12{};
		vk::PhysicalDeviceFeatures2 features2{};

		vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{};
		vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};

		// Chain them
		features2.pNext = &features12;
		features12.pNext = &features13;
		features13.pNext = &accelFeatures;
		accelFeatures.pNext = &rtPipelineFeatures;

		mPhysicalDevice.getFeatures2(&features2);

		evaluateFeature(features13.dynamicRendering, mConfig.mFeatures.dynamicRendering);
		evaluateFeature(features12.timelineSemaphore, mConfig.mFeatures.timelineSemaphore);
		evaluateFeature(features12.descriptorIndexing, mConfig.mFeatures.descriptorIndexing);
		evaluateFeature(features12.bufferDeviceAddress, mConfig.mFeatures.bufferDeviceAddress);
		evaluateFeature(features13.synchronization2, mConfig.mFeatures.synchronization2);

		// Ray tracing
		evaluateFeature(accelFeatures.accelerationStructure, mConfig.mFeatures.accelerationStructure);
		evaluateFeature(rtPipelineFeatures.rayTracingPipeline, mConfig.mFeatures.rayTracingPipeline);
	}

	bool VulkanRenderer::initialize(const RendererConfig& config)
	{
		mConfig = config;

		LOG_TRACE("VulkanRenderer::initialize. Width: {}, height: {}, headless {}, validation {}",
			mConfig.mWidth, mConfig.mHeight, mConfig.mHeadless, mConfig.mEnableValidation);

		if (volkInitialize() != VK_SUCCESS)
			return false;

		vk::detail::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr getProc =
			dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

		VULKAN_HPP_DEFAULT_DISPATCHER.init(getProc);

		std::vector<const char*> layers;
		if (mEnableValidation)
			layers.push_back("VK_LAYER_KHRONOS_validation");

		// Application
		vk::ApplicationInfo appInfo{};
		appInfo.pApplicationName = "FRE";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		LOG_TRACE("Vulkan API version: {}.{}.{}",
			VK_VERSION_MAJOR(appInfo.apiVersion),
			VK_VERSION_MINOR(appInfo.apiVersion),
			VK_VERSION_PATCH(appInfo.apiVersion));

		// Create instance
		vk::InstanceCreateInfo ici{};
		ici.pApplicationInfo = &appInfo;
		ici.enabledLayerCount = static_cast<uint32_t>(layers.size());
		ici.ppEnabledLayerNames = layers.data();

		// Extensions
		std::vector<const char*> extensions;
		if (mEnableValidation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		ici.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		ici.ppEnabledExtensionNames = extensions.data();

		{
			auto result = vk::createInstance(ici);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create instance");
			}
			mInstance = result.value;
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance);

		volkLoadInstance(mInstance);

		if (mEnableValidation)
		{
			createDebugMessenger();
		}

		// Select physical device
		auto gpus = mInstance.enumeratePhysicalDevices();
		mPhysicalDevice = gpus->front();

		// Select queue families
		if (!selectQueueFamilies())
			return false;

		std::set<uint32_t> uniqueFamilies =
		{
			mGraphicsQueueFamily,
			mComputeQueueFamily,
			mTransferQueueFamily
		};

		float priority = 1.0f;
		std::vector<vk::DeviceQueueCreateInfo> queueInfos;

		for (uint32_t family : uniqueFamilies)
		{
			vk::DeviceQueueCreateInfo qci{};
			qci.queueFamilyIndex = family;
			qci.queueCount = 1;
			qci.pQueuePriorities = &priority;

			queueInfos.push_back(qci);
		}

		// Create logical device
		vk::DeviceCreateInfo dci{};
		dci.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
		dci.pQueueCreateInfos = queueInfos.data();
		{
			auto result = mPhysicalDevice.createDevice(dci);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create device");
			}
			mDevice = result.value;
		}

		LOG_TRACE("VulkanRenderer::initialize. Vulkan device created. Graphics queue family: {}, Compute queue family: {}, Transfer queue family: {}",
			mGraphicsQueueFamily, mComputeQueueFamily, mTransferQueueFamily);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice);

		volkLoadDevice(mDevice);

		evaluateFeatures();

		return true;
	}

	void VulkanRenderer::shutdown()
	{
		LOG_TRACE("VulkanRenderer::shutdown");

		if (mDebugMessenger)
			mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger);

		if (mDevice)
		{
			mDevice.destroy();
		}

		if (mInstance)
		{
			mInstance.destroy();
		}
	}

	void VulkanRenderer::beginFrame()
	{
	}

	void VulkanRenderer::endFrame()
	{
	}

	void VulkanRenderer::waitIdle()
	{
		if (mDevice)
		{
			mDevice.waitIdle();
		}
	}
}