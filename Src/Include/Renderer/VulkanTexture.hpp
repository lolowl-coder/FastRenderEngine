#pragma once

namespace fre
{
	struct VulkanTexture
	{
		VkImage mImage = VK_NULL_HANDLE;
		VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;
		VkSamplerAddressMode mAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkImageTiling mTiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags mUsageFlags;
        VkMemoryPropertyFlags mMemoryFlags;
		VkImageLayout mLayout;
		Image mHostImage;
	};
}