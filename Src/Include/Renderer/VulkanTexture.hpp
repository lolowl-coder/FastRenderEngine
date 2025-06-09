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
        VkImageUsageFlags mUsageFlags = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
        VkMemoryPropertyFlags mMemoryFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFlags mStageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		Image mImage;

        bool operator ==(const VulkanTextureInfo& other) const
        {
            return
				mId == other.mId &&
                mAddressMode == other.mAddressMode &&
                mTiling == other.mTiling &&
                mUsageFlags == other.mUsageFlags &&
                mMemoryFlags == other.mMemoryFlags &&
                mLayout == other.mLayout &&
                mStageFlags == other.mStageFlags &&
                mImage == other.mImage;
        }
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