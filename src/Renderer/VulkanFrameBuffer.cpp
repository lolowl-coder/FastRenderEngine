#include "Renderer/VulkanFrameBuffer.hpp"
#include "Utilities.hpp"

#include <array>

namespace fre
{
    void VulkanFrameBuffer::create(const MainDevice& mainDevice,
        VkImageView swapChainImageView, VkExtent2D swapchainExtent,
        VkRenderPass renderPass)
    {
        createColourBufferImage(mainDevice, swapchainExtent);
        createDepthBufferImage(mainDevice, swapchainExtent);
        createFrameBuffers(mainDevice.logicalDevice, swapChainImageView,
            swapchainExtent, renderPass);
    }

    void VulkanFrameBuffer::destroy(VkDevice logicalDevice)
    {
        vkDestroyImageView(logicalDevice, mColourBufferImageView, nullptr);
        vkDestroyImage(logicalDevice, mColourBufferImage, nullptr);
        vkFreeMemory(logicalDevice, mColourBufferImageMemory, nullptr);

        vkDestroyImageView(logicalDevice, mDepthBufferImageView, nullptr);
        vkDestroyImage(logicalDevice, mDepthBufferImage, nullptr);
        vkFreeMemory(logicalDevice, mDepthBufferImageMemory, nullptr);

        vkDestroyFramebuffer(logicalDevice, mFrameBuffer, nullptr);
    }

	void VulkanFrameBuffer::createColourBufferImage(const MainDevice& mainDevice,
        VkExtent2D swapChainExtent)
	{
		//Get supported format for colour attachment
		VkFormat colourFormat = chooseSupportedFormat(
			mainDevice.physicalDevice,
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

        //Create colour buffer image
        mColourBufferImage = createImage(
            mainDevice,
            swapChainExtent.width, swapChainExtent.height,
            colourFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &mColourBufferImageMemory);

        //Create Colour Image View
        mColourBufferImageView = createImageView(
            mainDevice.logicalDevice, mColourBufferImage, colourFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanFrameBuffer::createDepthBufferImage(const MainDevice& mainDevice,
        VkExtent2D swapChainExtent)
	{
		//Get supported format for depth buffer
		VkFormat depthFormat = chooseSupportedFormat(
			mainDevice.physicalDevice,
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        //Create depth buffer image
        mDepthBufferImage = createImage(
            mainDevice,
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &mDepthBufferImageMemory);

        //Create Depth Image View
        mDepthBufferImageView = createImageView(
            mainDevice.logicalDevice, mDepthBufferImage, depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void VulkanFrameBuffer::createFrameBuffers(VkDevice logicalDevice,
        VkImageView swapChainImageView, VkExtent2D swapChainExtent,
        VkRenderPass renderPass)
	{
		//Create a framebuffer for each swap chain image
        std::array<VkImageView, 3> attachments =
        {
            swapChainImageView,
            mColourBufferImageView,
            mDepthBufferImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;	//Render pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();	//List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = swapChainExtent.width;
        framebufferCreateInfo.height = swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &mFrameBuffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Frame Buffer!");
        }
	}
}