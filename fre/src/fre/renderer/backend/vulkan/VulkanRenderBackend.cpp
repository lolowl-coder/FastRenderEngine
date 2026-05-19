#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Log.hpp"
#include "fre/renderer/ISurface.hpp"
#include "fre/renderer/backend/vulkan/IVulkanSurface.hpp"
#include "fre/renderer/backend/vulkan/VulkanContext.hpp"
#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommandPool.hpp"
#include "fre/renderer/backend/vulkan/VulkanExtension.hpp"
#include "fre/renderer/backend/vulkan/VulkanImage.hpp"
#include "fre/renderer/backend/vulkan/VulkanImageView.hpp"
#include "fre/renderer/backend/vulkan/VulkanRenderBackend.hpp"
#include "fre/renderer/backend/vulkan/VulkanSurfaceWindows.hpp"
#include "fre/renderer/backend/vulkan/VulkanShader.hpp"
#include "fre/renderer/backend/vulkan/VulkanSwapchain.hpp"

#include <format>
#include <memory>
#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace fre
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		using namespace fre;

		if(messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
		{
			LOG_ERROR(pCallbackData->pMessage);
		}
		else if(messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			LOG_WARNING(pCallbackData->pMessage);
		}
		else
		{
			LOG_INFO(pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	VulkanRenderBackend::VulkanRenderBackend(const CommonRendererConfig& commonConfig)
		: mCommonConfig(commonConfig)
	{
		initialize();
	}

	VulkanRenderBackend::~VulkanRenderBackend()
	{
		shutdown();
	}

	VulkanContext& VulkanRenderBackend::getContext()
	{
		auto context = dynamic_cast<VulkanContext*>(mCommonConfig.mContext);
		assert(context && "Context must be a VulkanContext");
		return *context;
	}

	bool VulkanRenderBackend::selectQueueFamilies()
	{
		auto families = mPhysicalDevice.getQueueFamilyProperties();

		int graphicsIndex = -1;
		int computeIndex = -1;
		int transferIndex = -1;
		int presentIndex = -1;

		int presentGraphics = -1;
		int presentCompute = -1;
		int presentAny = -1;

		int dedicatedCompute = -1;
		int dedicatedTransfer = -1;

		for(uint32_t i = 0; i < families.size(); ++i)
		{
			const auto& props = families[i];

			bool graphics = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eGraphics);
			bool mCompute = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eCompute);
			bool mTransfer = static_cast<bool>(props.queueFlags & vk::QueueFlagBits::eTransfer);
			bool mPresent = vkCheck(mPhysicalDevice.getSurfaceSupportKHR(i, mVkSurface->handle())) == VK_TRUE;

			if(graphics && graphicsIndex == -1)
				graphicsIndex = i;

			// Dedicated compute: compute but not graphics
			if(mCompute && !graphics)
				dedicatedCompute = i;

			// Dedicated transfer: transfer but not graphics and not compute
			if(mTransfer && !graphics && !mCompute)
				dedicatedTransfer = i;

			if(mCompute && computeIndex == -1)
				computeIndex = i;

			if(mTransfer && transferIndex == -1)
				transferIndex = i;
				
			if(mPresent)
			{
				if(graphics && presentGraphics == -1)
					presentGraphics = i;

				else if(mCompute && presentCompute == -1)
					presentCompute = i;

				else if(presentAny == -1)
					presentAny = i;
			}
		}

		if(graphicsIndex == -1)
			return false;

		if(presentGraphics != -1)
			presentIndex = presentGraphics;
		else if(presentCompute != -1)
			presentIndex = presentCompute;
		else
			presentIndex = presentAny;

		if(presentIndex == -1)
			return false;

		mQueueFamilySelection.mGraphics.familyIndex = graphicsIndex;

		// Prefer dedicated compute
		if(dedicatedCompute != -1)
			mQueueFamilySelection.mCompute.familyIndex = dedicatedCompute;
		else
			mQueueFamilySelection.mCompute.familyIndex = computeIndex;

		// Prefer dedicated transfer
		if(dedicatedTransfer != -1)
			mQueueFamilySelection.mTransfer.familyIndex = dedicatedTransfer;
		else
			mQueueFamilySelection.mTransfer.familyIndex = transferIndex;

		if(presentIndex != -1)
			mQueueFamilySelection.mPresent.familyIndex = presentIndex;

		mQueueFamilySelection.mGraphics.queueIndex = mQueueCounts[mQueueFamilySelection.mGraphics.familyIndex];
		mQueueCounts[mQueueFamilySelection.mGraphics.familyIndex]++;
		mQueueFamilySelection.mCompute.queueIndex = mQueueCounts[mQueueFamilySelection.mCompute.familyIndex];
		mQueueCounts[mQueueFamilySelection.mCompute.familyIndex]++;
		mQueueFamilySelection.mTransfer.queueIndex = mQueueCounts[mQueueFamilySelection.mTransfer.familyIndex];
		mQueueCounts[mQueueFamilySelection.mTransfer.familyIndex]++;
		mQueueFamilySelection.mPresent.queueIndex= mQueueCounts[mQueueFamilySelection.mPresent.familyIndex];
		mQueueCounts[mQueueFamilySelection.mPresent.familyIndex]++;

		return true;
	}

	bool VulkanRenderBackend::createDebugMessenger()
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

		mDebugMessenger = vkCheck(getContext().getInstance().createDebugUtilsMessengerEXT(createInfo));

		return true;
	}

	bool VulkanRenderBackend::createPipelineCache()
	{
		mPipelineCache = std::make_unique<VulkanPipelineCache>(mDevice, mAllocator.get());

		return mPipelineCache != nullptr;
	}

	bool VulkanRenderBackend::createFrames()
	{
		mFrames.clear();
		for(int i = 0; i < mFramesInFlight; i++)
		{
			mFrames.push_back(
				{
					.mCmdBuff = mGraphicsCommandPool->allocatePrimary(),
					.mRenderFence = Fence(mDevice),
					.mImageAvailable = Semaphore(mDevice),
					.mRenderFinished = Semaphore(mDevice)
				}
			);
		}

		return mFrames.size() == mFramesInFlight;
	}

	bool VulkanRenderBackend::evaluateFeatures(const vk::PhysicalDevice& physicalDevice)
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

		physicalDevice.getFeatures2(&features2);

		bool result =
			evaluateRequirement(features13.dynamicRendering, mDesc.mFeatures.dynamicRendering)
			&& evaluateRequirement(features12.timelineSemaphore, mDesc.mFeatures.timelineSemaphore)
			&& evaluateRequirement(features12.descriptorIndexing, mDesc.mFeatures.descriptorIndexing)
			&& evaluateRequirement(features12.bufferDeviceAddress, mDesc.mFeatures.bufferDeviceAddress)
			&& evaluateRequirement(features13.synchronization2, mDesc.mFeatures.synchronization2)
			// Ray tracing
			&& evaluateRequirement(accelFeatures.accelerationStructure, mDesc.mFeatures.accelerationStructure)
			&& evaluateRequirement(rtPipelineFeatures.rayTracingPipeline, mDesc.mFeatures.rayTracingPipeline);

		return result;
	}

	bool VulkanRenderBackend::evaluateExtensions(const vk::PhysicalDevice& physicalDevice)
	{
		auto deviceExtensions = vkCheck(physicalDevice.enumerateDeviceExtensionProperties());
		bool result = true;
		for(auto& ext : mDesc.mDeviceExtensions)
		{
			if(!evaluateRequirement(isExtensionSupported(deviceExtensions, ext), ext))
			{
				result = false;
				break;
			}
		}
		auto instanceExtensions = vkCheck(vk::enumerateInstanceExtensionProperties());
		for(auto& ext : mDesc.mInstanceExtensions)
		{
			if(!evaluateRequirement(isExtensionSupported(instanceExtensions, ext), ext))
			{
				result = false;
				break;
			}
		}

		return result;
	}

	int VulkanRenderBackend::scorePhysicalDevice(const vk::PhysicalDevice& device)
	{
		auto props = device.getProperties();
		auto mem = device.getMemoryProperties();

		int score = 0;

		if(props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 10000;

		if(props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			score += 1000;

		score += props.limits.maxImageDimension2D;

		uint64_t vram = 0;

		for(uint32_t i = 0; i < mem.memoryHeapCount; ++i)
		{
			if(mem.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
				vram += mem.memoryHeaps[i].size;
		}

		score += static_cast<int>(vram / (1024 * 1024 * 1024));

		return score;
	}

	bool VulkanRenderBackend::selectPhysicalDevice()
	{
		auto physicalDevices = vkCheck(getContext().getInstance().enumeratePhysicalDevices());
		LOG_INFO("GPUs found:");
		for(auto& physicalDevice : physicalDevices)
		{
			LOG_INFO("\033[36m{}\033[0m", physicalDevice.getProperties().deviceName);
		}

		if(mCommonConfig.mGPUSelectionMode == GPUSelectionMode::Index)
		{
			if(physicalDevices.size() > mCommonConfig.mGPUIndex)
			{
				mPhysicalDevice = physicalDevices[mCommonConfig.mGPUIndex];
				LOG_INFO("Selected GPU: {} based on user preference", mPhysicalDevice.getProperties().deviceName);
			}
			else
			{
				LOG_ERROR("GPU index {} is out of range. Available GPUs: {}", mCommonConfig.mGPUIndex, physicalDevices.size());
				return false;
			}
		}
		else if(mCommonConfig.mGPUSelectionMode == GPUSelectionMode::Auto)
		{
			int maxScore = -1;
			for(auto& physicalDevice : physicalDevices)
			{
				bool featuresSupported = evaluateFeatures(physicalDevice);
				if(!featuresSupported)
				{
					LOG_INFO("GPU: {} does not support required features and will be skipped", physicalDevice.getProperties().deviceName);
					continue;
				}
				bool extensionsSupported = evaluateExtensions(physicalDevice);
				if(!extensionsSupported)
				{
					LOG_INFO("GPU: {} does not support required extensions and will be skipped", physicalDevice.getProperties().deviceName);
					continue;
				}
				if(featuresSupported && extensionsSupported)
				{
					int score = scorePhysicalDevice(physicalDevice);
					if(score > maxScore)
					{
						maxScore = score;
						mPhysicalDevice = physicalDevice;
					}
					LOG_INFO("GPU: {}, Score: {}", mPhysicalDevice.getProperties().deviceName, score);
				}
			}
		}

		return true;
	}

	bool VulkanRenderBackend::createLogicalDevice()
	{
		std::set<uint32_t> uniqueFamilies =
		{
			mQueueFamilySelection.mGraphics.familyIndex,
			mQueueFamilySelection.mCompute.familyIndex,
			mQueueFamilySelection.mTransfer.familyIndex,
			mQueueFamilySelection.mPresent.familyIndex,
		};

		std::vector<float> priority(uniqueFamilies.size(), 1.0f);
		std::vector<vk::DeviceQueueCreateInfo> queueInfos;

		for(uint32_t family : uniqueFamilies)
		{
			vk::DeviceQueueCreateInfo qci{};
			qci.queueFamilyIndex = family;
			qci.queueCount = mQueueCounts[family];
			qci.pQueuePriorities = priority.data();

			queueInfos.push_back(qci);
		}

		// Create logical device
		vk::DeviceCreateInfo dci{};
		dci.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
		dci.pQueueCreateInfos = queueInfos.data();

		// Enable extensions
		std::vector<const char*> enabledExtensions;
		for(auto& ext : mDesc.mDeviceExtensions)
		{
			if(ext.enabled)
			{
				enabledExtensions.push_back(ext.name.c_str());
			}
		}
		dci.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		dci.ppEnabledExtensionNames = enabledExtensions.data();

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
		if(
			mDesc.mFeatures.dynamicRendering.enabled &&
			mDesc.mFeatures.dynamicRendering.requirement == Requirement::Required)
		{
			dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
			dynamicRenderingFeature.dynamicRendering = VK_TRUE;
			dci.pNext = &dynamicRenderingFeature;
		}

		mDevice = vkCheck(mPhysicalDevice.createDevice(dci));

		LOG_TRACE("Logical device created.");
		LOG_TRACE("Graphics queue family : \033[36m{}\033[0m, Compute queue family : \033[36m{}\033[0m, Transfer queue family : \033[36m{}\033[0m, Present queue family : \033[36m{}\033[0m",
			mQueueFamilySelection.mGraphics.familyIndex,
			mQueueFamilySelection.mCompute.familyIndex,
			mQueueFamilySelection.mTransfer.familyIndex,
			mQueueFamilySelection.mPresent.familyIndex);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice);

		volkLoadDevice(mDevice);

		return true;
	}

	bool VulkanRenderBackend::createAllocator()
	{
		mAllocator = std::make_unique<VulkanAllocator>(getContext().getInstance(), mPhysicalDevice, mDevice);

		return mAllocator != nullptr;
	}

	bool VulkanRenderBackend::createQueues()
	{
		mGraphicsQueue = std::make_unique<VulkanQueue>(
			mDevice,
			mQueueFamilySelection.mGraphics.familyIndex,
			mQueueFamilySelection.mGraphics.queueIndex);

		mComputeQueue = std::make_unique<VulkanQueue>(
			mDevice,
			mQueueFamilySelection.mCompute.familyIndex,
			mQueueFamilySelection.mCompute.queueIndex);

		mTransferQueue = std::make_unique<VulkanQueue>(
			mDevice,
			mQueueFamilySelection.mTransfer.familyIndex,
			mQueueFamilySelection.mTransfer.queueIndex);

		mPresentQueue = std::make_unique<VulkanQueue>(
			mDevice,
			mQueueFamilySelection.mPresent.familyIndex,
			mQueueFamilySelection.mPresent.queueIndex);

		return true;
	}

	bool VulkanRenderBackend::createPools()
	{
		mGraphicsCommandPool = std::make_unique<VulkanCommandPool>(
			mDevice, mQueueFamilySelection.mGraphics.familyIndex);

		mComputeCommandPool = std::make_unique<VulkanCommandPool>(
			mDevice, mQueueFamilySelection.mCompute.familyIndex);

		mTransferCommandPool = std::make_unique<VulkanCommandPool>(
			mDevice, mQueueFamilySelection.mTransfer.familyIndex);

		return true;
	}

	bool VulkanRenderBackend::initialize()
	{
		//TODO: read from file config
		mDesc.mFeatures.dynamicRendering.requirement = fre::Requirement::Required;
		mDesc.mFeatures.timelineSemaphore.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.bufferDeviceAddress.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.descriptorIndexing.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.synchronization2.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.accelerationStructure.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.rayTracingPipeline.requirement = fre::Requirement::Optional;
		mDesc.mFeatures.rayQuery.requirement = fre::Requirement::Optional;

		LOG_TRACE("Common renderer config: Width: {}, height: {}, headless {}, validation {}",
			mCommonConfig.mWidth, mCommonConfig.mHeight, mCommonConfig.mHeadless, mCommonConfig.mEnableValidation);

		if(mCommonConfig.mEnableValidation)
		{
			if(!createDebugMessenger())
				return false;
		}

		mVkSurface = dynamic_cast<IVulkanSurface*>(mCommonConfig.mSurface);
		assert(mVkSurface && "Surface must implement IVulkanSurface");

		// Select physical device
		if(!selectPhysicalDevice())
			return false;

		// Select queue families
		if(!selectQueueFamilies())
			return false;

		if(!createLogicalDevice())
			return false;

		if(!createAllocator())
			return false;

		if(!createQueues())
			return false;

		if(!createPools())
			return false;

		if(!createPipelineCache())
			return false;

		if(!createFrames())
			return false;

		VulkanSwapchain::Desc swapchainDesc =
		{
			.mPhysicalDevice = mPhysicalDevice,
			.mDevice = mDevice,
			.mSurface = mVkSurface->handle(),
			.mWidth = mCommonConfig.mWidth,
			.mHeight = mCommonConfig.mHeight,
			.mGraphicsQueueFamily = mQueueFamilySelection.mGraphics.familyIndex,
			.mPresentQueueFamily = mQueueFamilySelection.mPresent.familyIndex
		};

		mSwapchain = std::make_unique<VulkanSwapchain>(swapchainDesc);

		return true;
	}

	void VulkanRenderBackend::shutdown()
	{
		LOG_TRACE("VulkanRenderBackend::shutdown");

		waitIdle();

		auto instance = getContext().getInstance();
		if(mDebugMessenger)
			instance.destroyDebugUtilsMessengerEXT(mDebugMessenger);
	}

	void VulkanRenderBackend::waitIdle()
	{
		if(mDevice)
		{
			vkCheck(mDevice.waitIdle());
		}
	}

	void VulkanRenderBackend::drawFrame(IScene* scene, RenderPassData& renderPassData)
	{
		auto& frame = mFrames[mCurrentFrame];
		// 1. Wait for GPU to finish with this frame
		frame.mRenderFence.wait();

		const auto acquireResult = mSwapchain->acquire(frame.mImageAvailable);
		if(acquireResult.resized)
		{
			LOG_INFO("Swapchain resized, recreating...");
			mSwapchain->recreate(mCommonConfig.mWidth, mCommonConfig.mHeight);
			return;
		}

		// Reset fence BEFORE submitting new work
		frame.mRenderFence.reset();

		recordCommands(frame.mCmdBuff, acquireResult.imageIndex, scene, renderPassData);
		submit(frame);
		present(frame);

		// Advance frame index (VERY IMPORTANT)
		mCurrentFrame = (mCurrentFrame + 1) % mFramesInFlight;
	}

	PipelineKey VulkanRenderBackend::makeDefaultPipelineKey(
		IShader* shader)
	{
		PipelineKey key{};
		key.colorFormat = mSwapchain->format();
		key.depthFormat = vk::Format::eUndefined;
		key.shader = shader;
		return key;
	}

	void VulkanRenderBackend::recordFrame(VulkanCommandBuffer& cmdBuf, const uint32_t imageIndex, RenderPassData& renderPassData)
	{
		PipelineKey key = makeDefaultPipelineKey(renderPassData.shader);

		auto* pipeline = getPipeline(key);

		cmdBuf.get().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->handle);

		// Viewport/scissor (dynamic state)
		vk::Viewport viewport(
			0.0f, 0.0f,
			static_cast<float>(mSwapchain->extent().width),
			static_cast<float>(mSwapchain->extent().height),
			0.0f, 1.0f);
		cmdBuf.get().setViewport(0, viewport);

		// Draw
		cmdBuf.get().draw(3, 1, 0, 0);
	}

	void VulkanRenderBackend::recordCommands(VulkanCommandBuffer& cmdBuff, const uint32_t imageIndex, IScene* scene, RenderPassData& renderPassData)
	{
		auto imageView = mSwapchain->getImageView(imageIndex);
		auto vkImageView = dynamic_cast<VulkanImageView*>(imageView);
		auto extent = mSwapchain->extent();
		auto vkImage = vkImageView->image();
		cmdBuff.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		cmdBuff.transitionImage(
			vkImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		);

		cmdBuff.beginRendering(
			vkImageView->handle(),
			extent,
			vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 1.0f, 1.0f})
		);

		recordFrame(cmdBuff, imageIndex, renderPassData);

		cmdBuff.endRendering();

		cmdBuff.transitionImage(
			vkImage,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits::eColorAttachmentWrite,
			{},
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe
		);

		cmdBuff.end();
	}

	void VulkanRenderBackend::submit(const Frame& frame)
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { frame.mImageAvailable.get() };
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		VkCommandBuffer cmd = frame.mCmdBuff.get();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		VkSemaphore signalSemaphores[] = { frame.mRenderFinished.get() };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		mGraphicsQueue->submit(
			submitInfo,
			frame.mRenderFence.get()
		);
	}

	void VulkanRenderBackend::present(const Frame& frame)
	{
		mSwapchain->present(mPresentQueue->get(), frame.mRenderFinished);
	}

    GpuImagePtr VulkanRenderBackend::createGpuImage(const IGpuImage::Desc& desc)
    {
        return std::make_unique<VulkanImage>(desc, mAllocator.get());
    }

    GpuImageViewPtr VulkanRenderBackend::createGpuImageView(IGpuImage* image, const IGpuImageView::Desc& desc)
    {
		auto vkImage = dynamic_cast<VulkanImage*>(image);
        return std::make_unique<VulkanImageView>(mDevice, vkImage->handle(), desc);
    }

	ShaderPtr VulkanRenderBackend::createGpuShader(const std::string& name, std::unordered_map<ShaderStage, ShaderStageBlob>& blobs)
	{
		return std::make_unique<VulkanShader>(name, blobs, mDevice);
	}
}