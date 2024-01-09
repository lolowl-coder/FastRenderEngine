#include "Renderer/VulkanFrameBuffer.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <array>

namespace fre
{
    void VulkanFrameBuffer::create(const MainDevice& mainDevice,
        std::vector<VkImageView> attachmentsViews, VkExtent2D swapChainExtent,
        VkRenderPass renderPass)
    {
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;	//Render pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentsViews.size());
        framebufferCreateInfo.pAttachments = attachmentsViews.data();	//List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = swapChainExtent.width;
        framebufferCreateInfo.height = swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &mFrameBuffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Frame Buffer!");
        }
    }

    void VulkanFrameBuffer::addColorAttachment(const VulkanAttachment& colorAttachment)
    {
        mColorAttachments.push_back(colorAttachment);
    }
    
    void VulkanFrameBuffer::setDepthAttachment(const VulkanAttachment& depthAttachment)
    {
        mDepthAttachment = depthAttachment;
    }

    void VulkanFrameBuffer::destroy(VkDevice logicalDevice)
    {
        for(auto& colorAttachment : mColorAttachments)
        {
            colorAttachment.destroy(logicalDevice);
        }
        mColorAttachments.clear();
        mDepthAttachment.destroy(logicalDevice);
        vkDestroyFramebuffer(logicalDevice, mFrameBuffer, nullptr);
    }
}