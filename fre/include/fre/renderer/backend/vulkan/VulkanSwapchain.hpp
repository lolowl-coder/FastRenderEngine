#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/backend/vulkan/VulkanSemaphore.hpp"

namespace fre
{
	class VulkanSwapchain
	{
	public:
		struct Desc
		{
			vk::PhysicalDevice mPhysicalDevice;
			vk::Device mDevice;
			vk::SurfaceKHR mSurface;
			uint32_t mWidth = 0;
			uint32_t mHeight = 0;
			uint32_t mGraphicsQueueFamily = UINT32_MAX;
			uint32_t mPresentQueueFamily = UINT32_MAX;
		};

		struct SwapchainDetails
		{
			//Surface properties, e.g. image size/extent
			vk::SurfaceCapabilitiesKHR surfaceCapabilities;
			//Surface image formats, e.g RGBA and size of each color
			std::vector<vk::SurfaceFormatKHR> formats;
			//How images should be presented to screen
			std::vector<vk::PresentModeKHR> presentationModes;
		};

		struct AcquireResult
		{
			uint32_t imageIndex;
			bool resized;   // signals swapchain needs recreation
		};

		VulkanSwapchain(const Desc& desc);
		~VulkanSwapchain();
		virtual AcquireResult acquire(const Semaphore& imageAvailableSemaphore);
		bool present(vk::Queue presentQueue, const Semaphore& waitSemaphore);
		virtual void recreate(uint32_t w, uint32_t h);
		IGpuImage* getImage(uint32_t index) const { return mImages[index].get(); }
		IGpuImageView* getImageView(uint32_t index) const { return mImageViews[index].get(); }
		vk::Format format() const { return mFormat; }
		virtual vk::Extent2D extent() const { return mExtent; }
	private:
		SwapchainDetails getSwapchainDetails(vk::PhysicalDevice device, vk::SurfaceKHR surface);
		vk::Extent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
		vk::SurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
		vk::PresentModeKHR chooseBestPresentationMode(const std::vector<vk::PresentModeKHR>& modes);
		void createImages();
		void createSwapchain();
		void cleanupSwapchain();
	private:
		Desc mDesc;
		vk::SwapchainKHR mSwapchain = {};
		vk::Format mFormat = vk::Format::eUndefined;
		vk::Extent2D mExtent = { 0u, 0u };
		std::vector<GpuImagePtr> mImages;
		std::vector<GpuImageViewPtr> mImageViews;

		uint32_t mImageIndex = 0;
	};
}