#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Log.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include <volk.h>

#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace fre
{
	VulkanRenderer::VulkanRenderer()
	{
		mFS = createFileSystem();
		mVFS = std::make_unique<VirtualFileSystem>(*mFS);

		Log::initialize(*mFS, true, true);
	}

	bool VulkanRenderer::selectQueueFamilies()
	{
		auto families = m_physicalDevice.getQueueFamilyProperties();

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

		m_graphicsQueueFamily = graphicsIndex;

		// Prefer dedicated compute
		if (dedicatedCompute != -1)
			m_computeQueueFamily = dedicatedCompute;
		else
			m_computeQueueFamily = computeIndex;

		// Prefer dedicated transfer
		if (dedicatedTransfer != -1)
			m_transferQueueFamily = dedicatedTransfer;
		else
			m_transferQueueFamily = transferIndex;

		return true;
	}

	bool VulkanRenderer::initialize(const RendererDesc& desc)
	{
		LOG_TRACE("VulkanRenderer::initialize. Width: {}, height: {}, headless {}, validation {}", desc.width, desc.height, desc.headless, desc.enableValidation);

		m_enableValidation = desc.enableValidation;
		m_headless = desc.headless;

		if (volkInitialize() != VK_SUCCESS)
			return false;

		vk::detail::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr getProc =
			dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

		VULKAN_HPP_DEFAULT_DISPATCHER.init(getProc);

		std::vector<const char*> layers;
		if (m_enableValidation)
			layers.push_back("VK_LAYER_KHRONOS_validation");

		vk::ApplicationInfo appInfo{};
		appInfo.pApplicationName = "FRE";
		appInfo.apiVersion = VK_API_VERSION_1_3;

		LOG_TRACE("Vulkan API version: {}.{}.{}",
			VK_VERSION_MAJOR(appInfo.apiVersion),
			VK_VERSION_MINOR(appInfo.apiVersion),
			VK_VERSION_PATCH(appInfo.apiVersion));

		vk::InstanceCreateInfo ici{};
		ici.pApplicationInfo = &appInfo;
		ici.enabledLayerCount = static_cast<uint32_t>(layers.size());
		ici.ppEnabledLayerNames = layers.data();

		{
			auto result = vk::createInstance(ici);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create instance");
			}
			m_instance = result.value;
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

		volkLoadInstance(m_instance);

		auto gpus = m_instance.enumeratePhysicalDevices();
		m_physicalDevice = gpus->front();

		if (!selectQueueFamilies())
			return false;

		std::set<uint32_t> uniqueFamilies =
		{
			m_graphicsQueueFamily,
			m_computeQueueFamily,
			m_transferQueueFamily
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

		vk::DeviceCreateInfo dci{};
		dci.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
		dci.pQueueCreateInfos = queueInfos.data();
		{
			auto result = m_physicalDevice.createDevice(dci);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create device");
			}
			m_device = result.value;
		}

		LOG_TRACE("VulkanRenderer::initialize. Vulkan device created. Graphics queue family: {}, Compute queue family: {}, Transfer queue family: {}",
			m_graphicsQueueFamily, m_computeQueueFamily, m_transferQueueFamily);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

		volkLoadDevice(m_device);

		return true;
	}

	void VulkanRenderer::shutdown()
	{
		LOG_TRACE("VulkanRenderer::shutdown");

		if (m_device)
			m_device.destroy();

		if (m_instance)
			m_instance.destroy();
	}

	void VulkanRenderer::beginFrame()
	{
	}

	void VulkanRenderer::endFrame()
	{
	}

	void VulkanRenderer::waitIdle()
	{
		if (m_device)
			m_device.waitIdle();
	}
}