#include "Renderer/VulkanAttachment.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Utilities.hpp"

namespace fre
{
	std::vector<VkFormat> getImageFormats(EAttachmentKind attachmentKind)
	{
		std::vector<VkFormat> result;
		switch (attachmentKind)
		{
			case EAttachmentKind::Color: result = { VK_FORMAT_R8G8B8A8_UNORM };
				break;
			case EAttachmentKind::Color16: result = { VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
				break;
			case EAttachmentKind::Color32: result = { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT };
				break;
			case EAttachmentKind::DepthStencil: result = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT };
				break;
			//case AK_DEPTH_STENCIL: result = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT }; break;
			//case AK_DEPTH_STENCIL: result = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM }; break;
		}

		return result;
	}

	VkFormatFeatureFlags getFormatFeatureFlags(EAttachmentKind attachmentKind)
	{
		VkFormatFeatureFlags result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		switch (attachmentKind)
		{
			case EAttachmentKind::Color:
			case EAttachmentKind::Color16:
			case EAttachmentKind::Color32:
				result = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
				break;
			case EAttachmentKind::DepthStencil:
				result = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
			break;
		}

		return result;
	}

	VkImageUsageFlags getImageUsageFlags(EAttachmentKind attachmentKind)
	{
		VkImageUsageFlags result = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
		switch (attachmentKind)
		{
			case EAttachmentKind::Color: result = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			break;
			case EAttachmentKind::Color16:
			case EAttachmentKind::Color32: result = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;
			case EAttachmentKind::DepthStencil: result = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			break;
		}

		return result;
	}

	VkImageAspectFlags getImageAspectFlags(EAttachmentKind attachmentKind, VkFormat imageFormat)
	{
		VkImageAspectFlags result = VK_IMAGE_ASPECT_COLOR_BIT;
		switch (attachmentKind)
		{
			case EAttachmentKind::Color:
				result = VK_IMAGE_ASPECT_COLOR_BIT;
				break;
			case EAttachmentKind::DepthStencil:
				result = VK_IMAGE_ASPECT_DEPTH_BIT/* | VK_IMAGE_ASPECT_STENCIL_BIT*/;
				break;
			case EAttachmentKind::Count:
				result = VK_IMAGE_ASPECT_NONE;
				break;
		}

		return result;
	}

    void VulkanAttachment::create(const MainDevice& mainDevice, EAttachmentKind attachmentKind,
        VkExtent2D swapChainExtent)
	{
		LOG_INFO("Create framebuffer attachment. Attachment kind {}", static_cast<int>(attachmentKind));
		
		//Get supported format for color attachment
		VkFormat imageFormat = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			getImageFormats(attachmentKind),
			VK_IMAGE_TILING_OPTIMAL,
			getFormatFeatureFlags(attachmentKind)
		);

        //Create color buffer image
		uint32_t actualImageSize;
        mImage = createImage(
            mainDevice,
            swapChainExtent.width, swapChainExtent.height,
            imageFormat, VK_IMAGE_TILING_OPTIMAL,
            getImageUsageFlags(attachmentKind),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &mImageMemory, actualImageSize);

        //Create Color Image View
        mImageView = createImageView(
            mainDevice.logicalDevice, mImage, imageFormat,
            getImageAspectFlags(attachmentKind, imageFormat));
		
		LOG_INFO("Attachment created");
	}

    void fre::VulkanAttachment::destroy(VkDevice logicalDevice)
    {
        vkDestroyImageView(logicalDevice, mImageView, nullptr);
        vkDestroyImage(logicalDevice, mImage, nullptr);
        vkFreeMemory(logicalDevice, mImageMemory, nullptr);
    }
}
