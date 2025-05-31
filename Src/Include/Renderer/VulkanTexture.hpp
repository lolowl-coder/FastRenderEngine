#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include "Image.hpp"

namespace fre
{
	struct VulkanTextureInfo
	{
		uint32_t mId = std::numeric_limits<uint32_t>::max();
		VkSamplerAddressMode mAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkImageTiling mTiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags mUsageFlags;
        VkMemoryPropertyFlags mMemoryFlags;
		VkImageLayout mLayout;
		const std::vector<uint32_t>& mStageFlags;
		Image mImage;
	};

	struct VulkanTexture
	{
		uint32_t mId = std::numeric_limits<uint32_t>::max();
		VkImage mImage = VK_NULL_HANDLE;
		VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;
		//Actual size in GPU memory, bytes
		uint32_t mActualSize = 0;
	};
}