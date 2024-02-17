#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
	struct MainDevice;

	VkFormat chooseSupportedImageFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	VkImage createImage(const MainDevice& mainDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkDeviceMemory *imageMemory);

	VkImageView createImageView(VkDevice logicalDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void copyImageBuffer(VkDevice device, int8_t transferQueueFamilyId, int8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool transferCommandPool, VkBuffer srcBuffer,
		VkImage image, uint32_t width, uint32_t height);

	void transitionImageLayout(VkDevice device, VkQueue queue,
		VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
}