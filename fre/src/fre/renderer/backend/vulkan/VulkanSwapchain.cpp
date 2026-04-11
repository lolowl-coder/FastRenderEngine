#pragma once

#include "fre/core/Log.hpp"
#include "fre/renderer/backend/vulkan/VulkanSwapchain.hpp"
#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanImageView.hpp"

namespace fre
{
	VulkanSwapchain::SwapchainDetails VulkanSwapchain::getSwapchainDetails(
		vk::PhysicalDevice device,
		vk::SurfaceKHR surface)
	{
		SwapchainDetails details;

		details.surfaceCapabilities = vkCheck(device.getSurfaceCapabilitiesKHR(surface));

		details.formats = vkCheck(device.getSurfaceFormatsKHR(surface));

		details.presentationModes = vkCheck(device.getSurfacePresentModesKHR(surface));

		return details;
	}

	vk::Extent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if(surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surfaceCapabilities.currentExtent;
		}
		else
		{
			VkExtent2D newExtent = {};
			newExtent.width = static_cast<uint32_t>(mDesc.mWidth);
			newExtent.height = static_cast<uint32_t>(mDesc.mHeight);

			//Surface also defines max and min, so make sure within boundaries be clamping value
			newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
			newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

			return newExtent;
		}
	}

	vk::SurfaceFormatKHR VulkanSwapchain::chooseBestSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& formats)
	{
		if(formats.size() == 1 &&
			formats[0].format == vk::Format::eUndefined)
		{
			return { vk::Format::eR8G8B8A8Unorm,
					vk::ColorSpaceKHR::eSrgbNonlinear };
		}

		for(const auto& f : formats)
		{
			if(f.format == vk::Format::eR8G8B8A8Unorm &&
				f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				return f;
		}

		return formats[0];
	}

	vk::PresentModeKHR VulkanSwapchain::chooseBestPresentationMode(const std::vector<vk::PresentModeKHR>& modes)
	{
		for(auto m : modes)
		{
			if(m == vk::PresentModeKHR::eMailbox)
			{
				LOG_TRACE("Swapchain present mode: Mailbox");
				return m;
			}
		}

		LOG_TRACE("Swapchain present mode: FIFO");
		return vk::PresentModeKHR::eFifo;
	}

	void VulkanSwapchain::createImages()
	{
		auto images = vkCheck(mDesc.mDevice.getSwapchainImagesKHR(mSwapchain));

		mImageViews.clear();

		IGpuImageView::Desc desc = {};

		for(auto image : images)
		{
			mImageViews.push_back(std::make_unique<VulkanImageView>(mDesc.mDevice, image, desc));
		}
	}

	void VulkanSwapchain::createSwapchain()
	{
		auto swapDetails = getSwapchainDetails(mDesc.mPhysicalDevice, mDesc.mSurface);

		auto surfaceFormat = chooseBestSurfaceFormat(swapDetails.formats);
		auto presentMode = chooseBestPresentationMode(swapDetails.presentationModes);
		auto extent = chooseSwapExtent(swapDetails.surfaceCapabilities);

		uint32_t imageCount = swapDetails.surfaceCapabilities.minImageCount + 1;

		if(swapDetails.surfaceCapabilities.maxImageCount > 0 &&
			imageCount > swapDetails.surfaceCapabilities.maxImageCount)
		{
			imageCount = swapDetails.surfaceCapabilities.maxImageCount;
		}

		std::vector<uint32_t> queueFamilyIndices;

		vk::SharingMode sharingMode;

		if(mDesc.mGraphicsQueueFamily != mDesc.mPresentQueueFamily)
		{
			queueFamilyIndices = { mDesc.mGraphicsQueueFamily, mDesc.mPresentQueueFamily };
			sharingMode = vk::SharingMode::eConcurrent;
		}
		else
		{
			sharingMode = vk::SharingMode::eExclusive;
		}

		vk::SwapchainCreateInfoKHR createInfo(
			{},
			mDesc.mSurface,
			imageCount,
			surfaceFormat.format,
			surfaceFormat.colorSpace,
			extent,
			1u,
			vk::ImageUsageFlagBits::eColorAttachment,
			sharingMode,
			static_cast<uint32_t>(queueFamilyIndices.size()),
			queueFamilyIndices.data(),
			swapDetails.surfaceCapabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			presentMode,
			VK_TRUE
		);

		mSwapchain = vkCheck(mDesc.mDevice.createSwapchainKHR(createInfo));

		mFormat = surfaceFormat.format;
		mExtent = extent;

		createImages();
	}

	VulkanSwapchain::VulkanSwapchain(const Desc& desc)
		: mDesc(desc)
	{
		createSwapchain();
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		cleanupSwapchain();
	}

	VulkanSwapchain::AcquireResult VulkanSwapchain::acquire()
	{
		Frame& frame = mFrames[mCurrentFrame];

		// 1. Wait for GPU to finish with this frame
		frame.mRenderFence.wait();

		// 2. Acquire next image
		auto res = mDesc.mDevice.acquireNextImageKHR(
			mSwapchain,
			UINT64_MAX,
			frame.mImageAvailable.get(),
			VK_NULL_HANDLE
		);

		mImageIndex = res.value;
		
		// 3. Handle result
		if(res.result == vk::Result::eErrorOutOfDateKHR)
		{
			return { 0, nullptr, true };
		}

		if(res.result == vk::Result::eSuboptimalKHR)
		{
			// Not fatal — continue rendering, but mark for resize
			return { mImageIndex, &frame, true };
		}

		if(res.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to acquire swapchain image");
		}

		// 4. Reset fence BEFORE submitting new work
		frame.mRenderFence.reset();

		return { mImageIndex, &frame, false };
	}

	bool VulkanSwapchain::present(vk::Queue presentQueue)
	{
		Frame& frame = mFrames[mCurrentFrame];

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		vk::Semaphore waitSemaphores[] = { frame.mRenderFinished.get() };
		presentInfo.pWaitSemaphores = waitSemaphores;

		vk::SwapchainKHR swapchains[] = { mSwapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;

		presentInfo.pImageIndices = &mImageIndex;

		auto result = presentQueue.presentKHR(presentInfo);
		
		bool needResize = false;

		if(result == vk::Result::eErrorOutOfDateKHR)
		{
			needResize = true;
		}
		else if(result == vk::Result::eSuboptimalKHR)
		{
			needResize = true;
		}
		else if(result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to present swapchain image");
		}

		// Advance frame index (VERY IMPORTANT)
		mCurrentFrame = (mCurrentFrame + 1) % mFramesInFlight;

		return needResize;
	}

	void VulkanSwapchain::recreate(uint32_t width, uint32_t height)
	{
		if(width == 0 || height == 0)
			return; // minimized window case

		mDesc.mWidth = width;
		mDesc.mHeight = height;

		vkCheck(mDesc.mDevice.waitIdle());

		cleanupSwapchain();

		createSwapchain();
	}

	void VulkanSwapchain::cleanupSwapchain()
	{
		// Destroy swapchain images
		mImageViews.clear();

		if(mSwapchain)
		{
			mDesc.mDevice.destroySwapchainKHR(mSwapchain);
			mSwapchain = nullptr;
		}
	}
}