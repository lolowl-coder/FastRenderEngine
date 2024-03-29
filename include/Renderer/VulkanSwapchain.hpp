#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/VulkanImage.hpp"
#include "Utilities.hpp"

#include <vector>

namespace fre
{
	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;		//Surface properties, e.g. image size/extent
		std::vector<VkSurfaceFormatKHR> formats;			//Surface image formats, e.g RGBA and size of each color
		std::vector<VkPresentModeKHR> presentationModes;	//How images should be presented to screen
	};

	struct SwapChainImage
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE; 
	};

    struct VulkanSwapChain
    {
        void create(GLFWwindow* window, const MainDevice& mainDevice,
            int8_t graphicsQueueFamilyId, int8_t presentationQueueFamilyId,
            VkSurfaceKHR surface);
        SwapChainDetails getSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
        void destroy(VkDevice logicalDevice);
    public:
		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
		VkFormat mSwapChainImageFormat = VK_FORMAT_MAX_ENUM;
		VkExtent2D mSwapChainExtent = {0u, 0u};

		std::vector<SwapChainImage> mSwapChainImages;
    private:
        void createSwapChainImageViews(const MainDevice& mainDevice);
        void createColourBufferImage(const MainDevice& mainDevice);
        void createDepthBufferImage(const MainDevice& mainDevice);
        void createFrameBuffers(VkDevice logicalDevice, VkRenderPass renderPass);
		VkExtent2D chooseSwapExtent(GLFWwindow* window,
            const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
		VkSurfaceFormatKHR chooseBestSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& formats);
		VkPresentModeKHR chooseBestPresentationMode(
            const std::vector<VkPresentModeKHR>& presentationModes);
    };
}