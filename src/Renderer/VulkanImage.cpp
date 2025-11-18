#include "Renderer/VulkanImage.hpp"
#include "Utilities.hpp"

#ifdef _WIN64
	#include <VersionHelpers.h>
	#include <dxgi1_2.h>
	#include <aclapi.h>
	#define NOMINMAX
	#include <windows.h>
#endif /* _WIN64 */

#include <stdexcept>

using namespace glm;

namespace fre
{
	VkFormat chooseSupportedImageFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		//Loop through options an find compatible one
		for (VkFormat format : formats)
		{
			//Get properties for given format on this device
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			//Depending on tiling choise need to check for different bit flag
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
		}
		
		throw std::runtime_error("Failed to find a matching format!");
	}

	VkImage createExternalImage(const MainDevice& mainDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkExternalMemoryHandleTypeFlagsKHR extMemHandleType,
		VkDeviceMemory* imageMemory, uint32_t& actualSize)
	{
		//Create Image
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		//Layout of image on creation
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//What image will be used for
		imageCreateInfo.usage = useFlags;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		//Can't be shared between queues
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//Use extension as we need external memory
		VkExternalMemoryImageCreateInfo externalMemoryImageInfo = {};
		externalMemoryImageInfo.sType =
			VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
		externalMemoryImageInfo.handleTypes = extMemHandleType;
		imageCreateInfo.pNext = &externalMemoryImageInfo;

		VkImage image;
		VK_CHECK(vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image));

		//Create memory for image

		//Here we ask for memory requirements for this image
		VkMemoryRequirements memoryRequirement;
		vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirement);
		actualSize = memoryRequirement.size;

		#ifdef _WIN64
			WindowsSecurityAttributes winSecurityAttributes;

			VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
			vulkanExportMemoryWin32HandleInfoKHR.sType =
				VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
			vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
			vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
			vulkanExportMemoryWin32HandleInfoKHR.dwAccess =
				DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
			vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;
		#endif /* _WIN64 */
		VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
		vulkanExportMemoryAllocateInfoKHR.sType =
			VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
		#ifdef _WIN64
			vulkanExportMemoryAllocateInfoKHR.pNext =
				extMemHandleType & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR
				? &vulkanExportMemoryWin32HandleInfoKHR
				: NULL;
			vulkanExportMemoryAllocateInfoKHR.handleTypes = extMemHandleType;
		#else
			vulkanExportMemoryAllocateInfoKHR.pNext = NULL;
			vulkanExportMemoryAllocateInfoKHR.handleTypes =
				VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
		#endif /* _WIN64 */

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//Use extension to export memory
		memoryAllocInfo.pNext = &vulkanExportMemoryAllocateInfoKHR;
		memoryAllocInfo.allocationSize = memoryRequirement.size;
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memoryRequirement.memoryTypeBits, propFlags);

		VK_CHECK(vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory));

		//Connect memory to image
		VK_CHECK(vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0));

		return image;
	}

	VkImage createImage(const MainDevice& mainDevice,
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const uint32_t mipLevels,
		VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
		VkDeviceMemory* imageMemory, uint32_t& actualSize)
	{
		//Create Image
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Layout of image on creation
		imageCreateInfo.usage = useFlags;	//What image will be used for
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	//Can't be shared between queues

		VkImage image;
		VK_CHECK(vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image));

		//Create memory for image

		//Here we ask for memory requirements for this image
		VkMemoryRequirements memoryRequirement;
		vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirement);
		actualSize = memoryRequirement.size;

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirement.size;
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memoryRequirement.memoryTypeBits, propFlags);

		VK_CHECK(vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory));

		//Connect memory to image
		VK_CHECK(vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0));

		return image;
	}

	VkImageView createImageView(VkDevice logicalDevice,
		VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, const uint32_t mipLevelCount)
	{
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image;		//Image to create view for
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		// Allows remapping of RGBA components to other components
		if(
			format >= VK_FORMAT_R8_UNORM && format <= VK_FORMAT_R8_SRGB ||
			format >= VK_FORMAT_R16_UNORM && format <= VK_FORMAT_R16_SFLOAT ||
			format >= VK_FORMAT_R32_UINT && format <= VK_FORMAT_R32_SFLOAT ||
			format >= VK_FORMAT_R64_UINT && format <= VK_FORMAT_R64_SFLOAT)
		{
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_R;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_R;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;
		}
		else
		{
			viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		}

		//Subresource allow the view to view only a part of an image
		viewCreateInfo.subresourceRange.aspectMask = aspectFlags;		//Which aspect of image to view (COLOR_BIT, etc.)
		viewCreateInfo.subresourceRange.baseMipLevel = 0;				//Starting mip-level to view image from
		viewCreateInfo.subresourceRange.levelCount = mipLevelCount;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;				//Starting array layer to view from
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK(vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &imageView));

		return imageView;
	}

	void addImageBarrier(VkImage image, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		bool isDepth, VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.image = image;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccessFlags;
		barrier.dstAccessMask = dstAccessFlags;

		vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier );
	}

	void copyImageBuffer(VkDevice device, int8_t transferQueueFamilyId, int8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool transferCommandPool, VkBuffer srcBuffer,
		VkImage image, uint32_t width, uint32_t height)
	{
		//Create buffer
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

		VkBufferImageCopy imageRegion = {};
		imageRegion.bufferOffset = 0;	//Offset into data
		imageRegion.bufferRowLength = 0;	//Row length of data to calculate data spacing
		imageRegion.bufferImageHeight = 0;	//Image height to calculate data spacing
		imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	//Which aspect of image to copy
		imageRegion.imageSubresource.mipLevel = 0;	//Mip level to copy
		imageRegion.imageSubresource.baseArrayLayer = 0;	//Starting array layer (if presented)
		imageRegion.imageSubresource.layerCount = 1;	//Number of layers to copy starting at baseArrayLayer
		imageRegion.imageOffset = { 0, 0, 0 };	//Start origin (xyz)
		imageRegion.imageExtent = { width, height, 1 };	//Region size to copy (xyz)

		addImageBarrier(image,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			false, transferCommandBuffer);

		//Copy buffer to image
		vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

		addImageBarrier(image,
			transferQueueFamilyId, graphicsQueueFamilyId,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			false, transferCommandBuffer);

		endAndSubmitCommitBuffer(device, transferCommandPool, queue, transferCommandBuffer);
	}

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
		const uint32_t mipLevelCount)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = oldLayout;	//Lyaout to transition from
		imageMemoryBarrier.newLayout = newLayout;	//Lyaout to transition to
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	//Family to transition from
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	//Family to transition to
		imageMemoryBarrier.image = image;	//Image being accessed and modified as part of barrier
		imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
		imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevel;	//Mip level to start alternation on
		imageMemoryBarrier.subresourceRange.levelCount = mipLevelCount;	//First mip level to start alternation on
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;	//First layer to start alternation on
		imageMemoryBarrier.subresourceRange.layerCount = 1;	//Number of layers to start alternation on

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;
		if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;	//Memory access stage transition must happen after ...
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	//Memory access stage transition must happen before ...

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	//Memory access stage transition must happen after ...
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;	//Memory access stage transition must happen before ...

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;	//Memory access stage transition must happen after ...
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;	//Memory access stage transition must happen before ...

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;	//Memory access stage transition must happen after ...
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;	//Memory access stage transition must happen before ...

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		//from transfer destination to shader readable
		else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}

		if(srcStage != VK_PIPELINE_STAGE_NONE && dstStage != VK_PIPELINE_STAGE_NONE)
		{
			vkCmdPipelineBarrier(commandBuffer,
				srcStage, dstStage,	//Pipeline stages (match to src and dst AccessMasks)
				0,		//Dependency flags
				0, nullptr,	//Memory Barrier count + data
				0, nullptr,	//Buffer Memory Barrier count + data
				1, &imageMemoryBarrier	//Buffer Image Memory Barrier count + data
				);
		}
		else
		{
			LOG_ERROR("Can't transition image layout from {} to {}", oldLayout, newLayout);
		}
	}

	void transitionImageLayout(
		const VkDevice device,
		const VkQueue queue,
		const VkCommandPool commandPool,
		const VkImage image,
		const VkImageAspectFlags aspectMask,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const uint32_t mipLevel,
		const uint32_t mipLevelCount)
	{
		//Create buffer
		VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

        transitionImageLayout(device, queue, commandPool, commandBuffer, image, aspectMask, oldLayout, newLayout,
			mipLevel, mipLevelCount);

		endAndSubmitCommitBuffer(device, commandPool, queue, commandBuffer);
	}

	uint32_t getMipLevelCount(const ivec2& dimensions)
	{
		return 1;
		// Currently not used
		//return static_cast<uint32_t>(std::floor(std::log2(std::max(dimensions.x, dimensions.y)))) + 1;
	}

	void generateMipmaps(
		const VkDevice logicalDevice,
		const VkCommandPool cmdPool,
		const VkQueue queue,
		const VkImage image,
		const int32_t texWidth,
		const int32_t texHeight,
		const uint32_t mipLevels,
		const VkImageLayout dstLayout)
	{
		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		//Create buffer
		VkCommandBuffer commandBuffer = beginCommandBuffer(logicalDevice, cmdPool);

		for(uint32_t i = 1; i < mipLevels; i++)
		{
			// Transition i-1 level to SRC_OPTIMAL
			transitionImageLayout(
				logicalDevice, queue, cmdPool, commandBuffer, image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				i - 1u, 1u);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = {
				mipWidth > 1 ? mipWidth / 2 : 1,
				mipHeight > 1 ? mipHeight / 2 : 1,
				1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			// After blit, previous mip can be transitioned for shader read
			transitionImageLayout(
				logicalDevice, queue, cmdPool, commandBuffer, image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstLayout,
				i - 1u, 1u);
			
			mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
			mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
		}

		transitionImageLayout(
			logicalDevice, queue, cmdPool, commandBuffer, image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			dstLayout,
			mipLevels - 1u, 1u);

		endAndSubmitCommitBuffer(logicalDevice, cmdPool, queue, commandBuffer);
	}
}