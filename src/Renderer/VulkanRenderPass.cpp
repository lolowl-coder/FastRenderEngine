#include "Renderer/VulkanAttachment.hpp"
#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Log.hpp"
#include "Utilities.hpp"

#include <array>

using namespace glm;

namespace fre
{
    void VulkanRenderPass::create(const MainDevice& mainDevice, VkFormat swapChainImageFormat)
    {
		LOG_INFO("Create render pass");
		//ATTACHMENTS
		//SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

		//Color Attachment (Input)
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			getImageFormats(COLOR_ATTACHMENT),
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//Before render pass
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Don't care after render pass
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Image data layout after render pass (to change to)

		//Depth attachment (Input)
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = chooseSupportedImageFormat(
			mainDevice.physicalDevice,
			getImageFormats(DEPTH_ATTACHMENT),
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//We need to store after rendering to present image on screen
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//Color Attachment (Input) Reference
		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 1;	//This is attachment index in (std::array<VkImageView, 3> attachments) in createFrameBuffers()
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Layout before subpass

		//Depth Attachment Reference
		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 2;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//Layout before subpass

		//Array of subpasses
		std::array<VkSubpassDescription, 2> subpasses{};
		
		std::vector<VkAttachmentReference> colorAttachmentRefs = {colorAttachmentReference};

		//Setup subpass 1
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[0].colorAttachmentCount = colorAttachmentRefs.size();
		subpasses[0].pColorAttachments = colorAttachmentRefs.data();
		subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

		//2 ATTACHMENTS + REFERENCES

		//Swapchain color attachment
		VkAttachmentDescription swapchainColorAttachment = {};
		swapchainColorAttachment.format = swapChainImageFormat;
		swapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//We need to store after rendering to present image on screen
		swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		
		//Framebuffer data will be stored as an image, but images can be given different data layouts
		//to give optimal use for certain operations
		swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	//Image data layout after render pass (to change to)

		//Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
		VkAttachmentReference swapchainColorAttachmentReference = {};
		swapchainColorAttachmentReference.attachment = 0;
		swapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//References to attachments that subpass will take input from
		std::array<VkAttachmentReference, 1> inputReferences;
		inputReferences[0].attachment = 1;	//Color
		inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass
		//inputReferences[1].attachment = 2;	//Depth
		//inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass

		//Setup subpass 2
		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &swapchainColorAttachmentReference;
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

		/*subpassDependencies[3].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[3].dstSubpass = 1;
		subpassDependencies[3].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		subpassDependencies[3].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[3].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[3].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[4].srcSubpass = 1;
		subpassDependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[4].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[4].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		subpassDependencies[4].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[4].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;*/

		//That was color attachments dependencies. Now add depth dependencies
		/*subpassDependencies[3].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[3].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[3].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		//But must happen before
		subpassDependencies[3].dstSubpass = 0;
		subpassDependencies[3].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[3].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[4].srcSubpass = 0;
		subpassDependencies[4].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[4].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[4].dstSubpass = 1;
		subpassDependencies[4].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependencies[4].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpassDependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		//Convertion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		//Transition must happen after...
		subpassDependencies[5].srcSubpass = 1;
		subpassDependencies[5].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpassDependencies[5].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//But must happen before
		subpassDependencies[5].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[5].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[5].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[5].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;*/

		std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColorAttachment, colorAttachment, depthAttachment };

		//Create info for render pass
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
		renderPassCreateInfo.pAttachments = renderPassAttachments.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassCreateInfo.pSubpasses = subpasses.data();
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassCreateInfo.pDependencies = subpassDependencies.data();

		VK_CHECK(vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass));

		LOG_INFO("Render pass created");
    }

    void VulkanRenderPass::begin(
        VkFramebuffer swapChainFrameBuffer,
        VkExtent2D swapChainExtent, VkCommandBuffer commandBuffer,
		const vec4& clearColor)
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
