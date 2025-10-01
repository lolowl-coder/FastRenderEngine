#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Image.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace fre
{
	struct MainDevice;

	VkFormat chooseSupportedImageFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	VkImage createExternalImage(const MainDevice& mainDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkExternalMemoryHandleTypeFlagsKHR extMemHandleType,
		VkDeviceMemory *imageMemory, uint32_t& actualSize);
	
	VkImage createImage(const MainDevice& mainDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const uint32_t mipLevels,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkDeviceMemory *imageMemory, uint32_t& actualSize);

	VkImageView createImageView(VkDevice logicalDevice, VkImage image, VkFormat format,
		VkImageAspectFlags aspectFlags, const uint32_t mipLevelCount);

	void copyImageBuffer(VkDevice device, int8_t transferQueueFamilyId, int8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool transferCommandPool, VkBuffer srcBuffer,
		VkImage image, uint32_t width, uint32_t height);

	// Transition using external command buffer
	void transitionImageLayout(
		const VkDevice device,
		const VkQueue queue,
		const VkCommandPool commandPool,
		const VkCommandBuffer commandBuffer,
		const VkImage image,
		const VkImageAspectFlags aspectMask,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const uint32_t mipLevel,
		const uint32_t mipLevelCount);

    // Transition using internal command buffer
	void transitionImageLayout(
		const VkDevice device,
		const VkQueue queue,
		const VkCommandPool commandPool,
		const VkImage image,
		const VkImageAspectFlags aspectMask,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const uint32_t mipLevel,
		const uint32_t mipLevelCount);

	uint32_t getMipLevelCount(const glm::ivec2& dimensions);

	void generateMipmaps(
		const VkDevice logicalDevice,
		const VkCommandPool cmdPool,
		const VkQueue queue,
		const VkImage image,
		const int32_t texWidth,
		const int32_t texHeight,
		const uint32_t mipLevels,
		const VkImageLayout dstLayout);
}