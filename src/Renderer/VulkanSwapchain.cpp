#include "Renderer/VulkanSwapchain.hpp"

#include <array>

namespace fre
{
	void VulkanSwapChain::create(GLFWwindow* window, const MainDevice& mainDevice,
        VkSurfaceKHR surface)
	{
		//Get swap chain details so we can pick best settings
		SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice, surface);

		//Find optimal surface values for our swap chain

		//1. CHOOSE BEST SURFACE FORMAT
		VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
		//2. CHOOSET BEST PRESENTATION MODE
		VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
		//3. CHOOSE SWAP CHAIN IMAGE RESOLUTION
		VkExtent2D extent = chooseSwapExtent(window, swapChainDetails.surfaceCapabilities);

		//How many images are in swap chain? Get 1 more than the minimum to allow tripple buffering
		uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

		//If there is limit of max images (0 - limitless)
		//Clamp by max
		if (swapChainDetails.surfaceCapabilities.maxImageCount > 0
			&& swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
		{
			imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
		}
		//Creation information for swap chain
		VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = surface;
		swapChainCreateInfo.imageFormat = surfaceFormat.format;
		swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.imageExtent = extent;
		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageArrayLayers = 1;				//Number of layers for each image in chain
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//What attachment images will be used as
		swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;	//Transform to perform on swap chain images
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		//How to handle blending images with external graphics (e. g. other windows)
		swapChainCreateInfo.clipped = VK_TRUE;		//Where to clip parts of image not in view (e.g. ghind another window, off screen, etc.)

		//Get queue family indices
		QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice, surface);
		//If Graphics and Presentation families are different, then swap chain must let images be shared between families
		if (indices.graphicsFamily != indices.presentationFamily)
		{
			//Queues to share between
			uint32_t queueFamilyIndices[] =
			{
				(uint32_t)indices.graphicsFamily,
				(uint32_t)indices.presentationFamily
			};
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	//Image share handling
			swapChainCreateInfo.queueFamilyIndexCount = 2;						//Number of queues to share images between
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;		//Array of queues to share between
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		//If old swap chain been destroyed and this one replace it, then ling old one to quickly hand over responsibilities
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		//Create swapchain
		VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &mSwapChain);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a swapchain!");
		}

		//Store for later reference
		mSwapChainImageFormat = surfaceFormat.format;
		mSwapChainExtent = extent;

        createSwapChainImageViews(mainDevice);
	}

    void VulkanSwapChain::destroy(VkDevice logicalDevice)
    {
		for (auto image : mSwapChainImages) {
            vkDestroyImageView(logicalDevice, image.imageView, nullptr);
        }

		vkDestroySwapchainKHR(logicalDevice, mSwapChain, nullptr);
    }

	void VulkanSwapChain::createSwapChainImageViews(const MainDevice& mainDevice)
	{
		//Get swap chain images
		uint32_t swapChainImageCount;
		vkGetSwapchainImagesKHR(mainDevice.logicalDevice, mSwapChain, &swapChainImageCount, nullptr);
		std::vector<VkImage> images(swapChainImageCount);
		vkGetSwapchainImagesKHR(mainDevice.logicalDevice, mSwapChain, &swapChainImageCount, images.data());

		mSwapChainImages.clear();
		for (VkImage image : images)
		{
			//Store image handle
			SwapChainImage swapChainImage = {};
			swapChainImage.image = image;
			swapChainImage.imageView = createImageView(
                mainDevice.logicalDevice, image, mSwapChainImageFormat,
                VK_IMAGE_ASPECT_COLOR_BIT);

			//Add to swapchain image list
			mSwapChainImages.push_back(swapChainImage);
		}
	}

	SwapChainDetails VulkanSwapChain::getSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapChainDetails swapChainDetails;

		//-- CAPABILITIES --
		//Get the surface capabilities for the given surface on the given physical device
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

		//-- FORMATS --
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			swapChainDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
		}

		//-- PRESENTATION MODES --
		uint32_t presentationCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

		if (presentationCount != 0)
		{
			swapChainDetails.presentationModes.resize(presentationCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
		}

		return swapChainDetails;
	}

	VkExtent2D VulkanSwapChain::chooseSwapExtent(GLFWwindow* window,
        const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surfaceCapabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D newExtent = {};
			newExtent.width = static_cast<uint32_t>(width);
			newExtent.height = static_cast<uint32_t>(height);

			//Surface also defines max and min, so make sure within boundaries be clamping value
			newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
			newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

			return newExtent;
		}
	}

	//Best format is subjective, but ours will be:
	//format : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
	//colorSpace : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	VkSurfaceFormatKHR VulkanSwapChain::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		//If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		//If restricted, search for optimal format
		for (const auto& format : formats)
		{
			if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
				&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		//If can't find optimal format, then just return first format
		return formats[0];
	}

	VkPresentModeKHR VulkanSwapChain::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
	{
		//Look for Mailbox presentation mode
		for (const auto& presentationMode : presentationModes)
		{
			if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentationMode;
			}
		}

		//Default mode that is always available in Vulkan
		return VK_PRESENT_MODE_FIFO_KHR;
	}
}