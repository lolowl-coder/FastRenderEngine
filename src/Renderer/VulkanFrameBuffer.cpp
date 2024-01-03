#include "Renderer/VulkanFrameBuffer.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <array>

namespace fre
{
    void VulkanFrameBuffer::create(const MainDevice& mainDevice,
        VkImageView swapChainImageView, VkExtent2D swapChainExtent,
        VkRenderPass renderPass)
    {
        mColorAttachment.create(mainDevice, EAttachmentKind::AK_COLOR, swapChainExtent);
        mDepthAttachment.create(mainDevice, EAttachmentKind::AK_DEPTH, swapChainExtent);
        createFrameBuffers(mainDevice.logicalDevice, swapChainImageView,
            swapChainExtent, renderPass);
    }

    void VulkanFrameBuffer::destroy(VkDevice logicalDevice)
    {
        mColorAttachment.destroy(logicalDevice);
        mDepthAttachment.destroy(logicalDevice);

        vkDestroyFramebuffer(logicalDevice, mFrameBuffer, nullptr);
    }

	void VulkanFrameBuffer::createFrameBuffers(VkDevice logicalDevice,
        VkImageView swapChainImageView, VkExtent2D swapChainExtent,
        VkRenderPass renderPass)
	{
		//Create a framebuffer for each swap chain image
        std::array<VkImageView, 3> attachments =
        {
            swapChainImageView,
            mColorAttachment.mImageView,
            mDepthAttachment.mImageView
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