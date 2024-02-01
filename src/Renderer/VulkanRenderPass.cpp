#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Utilities.hpp"

#include <array>

namespace fre
{
    void VulkanRenderPass::create(const MainDevice& mainDevice, VkFormat swapChainImageFormat)
    {
		//ATTACHMENTS
		//SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

		//Colour Attachment (Input)
		VkAttachmentDescription colourAttachment = {};
		colourAttachment.format = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//Before render pass
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Don't care after render pass
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Image data layout after render pass (to change to)

		//Depth attachment (Input)
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//We need to store after rendering to present image on screen
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//Colour Attachment (Input) Reference
		VkAttachmentReference colourAttachmentReference = {};
		colourAttachmentReference.attachment = 1;	//This is attachment index in (std::array<VkImageView, 3> attachments) in createFrameBuffers()
		colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Layout before subpass

		//Depth Attachment (Input) Reference
		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 2;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//Layout before subpass

		//Array of subpasses
		std::array<VkSubpassDescription, 2> subpasses{};
		
		//Setup subpass 1
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[0].colorAttachmentCount = 1;
		subpasses[0].pColorAttachments = &colourAttachmentReference;
		subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

		//2 ATTACHMENTS + REFERENCES

		//Swapchain colour attachment
		VkAttachmentDescription swapchainColourAttachment = {};
		swapchainColourAttachment.format = swapChainImageFormat;
		swapchainColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		swapchainColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//We need to store after rendering to present image on screen
		swapchainColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		swapchainColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		
		//Framebuffer data will be stored as an image, but images can be given different data layouts
		//to give optimal use for certain operations
		swapchainColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		swapchainColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	//Image data layout after render pass (to change to)

		//Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
		VkAttachmentReference swapchainColourAttachmentReference = {};
		swapchainColourAttachmentReference.attachment = 0;
		swapchainColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//References to attachments that subpass will take input from
		std::array<VkAttachmentReference, 2> inputReferences;
		inputReferences[0].attachment = 1;	//Colour
		inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass
		inputReferences[1].attachment = 2;	//Depth
		inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass

		//Setup subpass 2
		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &swapchainColourAttachmentReference;
		subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
		subpasses[1].pInputAttachments = inputReferences.data();

		//SUBPASS DEPENDENCIES
		//Need to determine when layout transitions occur using subpass dependencies
		std::array<VkSubpassDependency, 3> subpassDependencies;

		//Convertion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		//Transition must happen after...
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;		//Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		//Pipeline stage
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				//Stage access mask (memory access)
		//But must happen before
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;

		//Subpass 1 layout (color/depth) to Subpass 2 layout (shader read)
		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstSubpass = 1;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = 0;

		//Convertion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		//Transition must happen after...
		subpassDependencies[2].srcSubpass = 0;
		subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//But must happen before
		subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[2].dependencyFlags = 0;

		std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColourAttachment, colourAttachment, depthAttachment };

		//Create info for render pass
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
		renderPassCreateInfo.pAttachments = renderPassAttachments.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassCreateInfo.pSubpasses = subpasses.data();
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassCreateInfo.pDependencies = subpassDependencies.data();

		VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
    }

    void VulkanRenderPass::begin(
        VkFramebuffer swapChainFrameBuffer,
        VkExtent2D swapChainExtent, VkCommandBuffer commandBuffer,
		const glm::vec4& clearColor)
    {
        std::array<VkClearValue, 3> clearValues = {};
		clearValues[0].color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
		clearValues[1].color = {clearColor.r, clearColor.g, clearColor.b, clearColor.a};
		clearValues[2].depthStencil.depth = 1.0f;

		//Information about how to begin render pass (only need for graphical applications)
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = mRenderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };	//Start point in pixels
		renderPassBeginInfo.renderArea.extent = swapChainExtent;
		renderPassBeginInfo.pClearValues = clearValues.data();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.framebuffer = swapChainFrameBuffer;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderPass::end(VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }

    void VulkanRenderPass::destroy(VkDevice logicalDevice)
    {
        vkDestroyRenderPass(logicalDevice, mRenderPass, nullptr);
    }
}
