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
			case AK_COLOR: result = { VK_FORMAT_R8G8B8A8_UNORM };
			break;
			case AK_DEPTH: result = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
			break;
		}

		return result;
	}

	VkFormatFeatureFlags getFormatFeatureFlags(EAttachmentKind attachmentKind)
	{
		VkFormatFeatureFlags result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		switch (attachmentKind)
		{
			case AK_COLOR: result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
			case AK_DEPTH: result = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
			break;
		}

		return result;
	}

	VkImageUsageFlags getImageUsageFlags(EAttachmentKind attachmentKind)
	{
		VkImageUsageFlags result = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		switch (attachmentKind)
		{
			case AK_COLOR: result = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			break;
			case AK_DEPTH: result = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			break;
		}

		return result;
	}

	VkImageAspectFlags getImageAspectFlags(EAttachmentKind attachmentKind)
	{
		VkImageAspectFlags result = VK_IMAGE_ASPECT_COLOR_BIT;
		switch (attachmentKind)
		{
			case AK_COLOR: result = VK_IMAGE_ASPECT_COLOR_BIT;
			break;
			case AK_DEPTH: result = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		}

		return result;
	}

    void VulkanAttachment::create(const MainDevice& mainDevice, EAttachmentKind attachmentKind,
        VkExtent2D swapChainExtent)
	{
		//Get supported format for colour attachment
		VkFormat imageFormat = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			getImageFormats(attachmentKind),
			VK_IMAGE_TILING_OPTIMAL,
			getFormatFeatureFlags(attachmentKind)
		);

        //Create colour buffer image
        mImage = createImage(
            mainDevice,
            swapChainExtent.width, swapChainExtent.height,
            imageFormat, VK_IMAGE_TILING_OPTIMAL,
            getImageUsageFlags(attachmentKind),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &mImageMemory);

        //Create Colour Image View
        mImageView = createImageView(
            mainDevice.logicalDevice, mImage, imageFormat,
            getImageAspectFlags(attachmentKind));
	}

    void fre::VulkanAttachment::destroy(VkDevice logicalDevice)
    {
        vkDestroyImageView(logicalDevice, mImageView, nullptr);
        vkDestroyImage(logicalDevice, mImage, nullptr);
        vkFreeMemory(logicalDevice, mImageMemory, nullptr);
    }
}
