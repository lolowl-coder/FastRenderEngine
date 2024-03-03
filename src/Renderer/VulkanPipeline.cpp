#include "Renderer/VulkanPipeline.hpp"

#include "Renderer/VulkanShader.hpp"

#include <algorithm>
#include <stdexcept>

namespace fre
{
    void VulkanPipeline::create(
		VkDevice logicalDevice,
		std::vector<VulkanShader> shaders,
		uint32_t stride,
        const std::vector<VulkanVertexAttribute>& vertexAttributes,
		VkBool32 depthWriteEnable,
		VkPolygonMode polygonMode,
		VkRenderPass renderPass,
		uint32_t subpassIndex,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstantRanges)
    {
        //Put shader stage creation info in to container
		//Graphics Pipeline creation info requires array of shader stage creates
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        for(auto shader : shaders)
        {
            VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
            shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            //Shader stage name
            shaderStageCreateInfo.stage = shader.mShaderStage;
            //Shader module to be used by stage
            shaderStageCreateInfo.module = shader.mShaderModule;
            //Entry point in to shader
            shaderStageCreateInfo.pName = "main";

            shaderStages.push_back(shaderStageCreateInfo);
        }

		//How the data for an attribute is defined within a vertex
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        attributeDescriptions.resize(vertexAttributes.size());
        for(uint32_t i = 0; i < attributeDescriptions.size(); i++)
        {
            attributeDescriptions[i].binding = 0;	//Which binding the data is at (should be the same as above)
            attributeDescriptions[i].location = i;	//Location in shader where data will be read from
            attributeDescriptions[i].format = vertexAttributes[i].mFormat;	//Data format
            attributeDescriptions[i].offset = vertexAttributes[i].mOffset;	//Where this attribute is defined in the data for a single vertex
        }

		// -- VERTEX INPUT --

        //How data for the single vertex (position, color, tex coords, normals, etc.) is as whole
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;	//Can bind multiple streams of data, this defines which one
		bindingDescription.stride = stride;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	//How to move between data after each vertex (instanced rendering or not, etc.)

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = stride == 0 ? 0 : 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = stride == 0 ? nullptr : &bindingDescription;		//List of vertex binding descriptions (data spacing/stride info)
		vertexInputCreateInfo.vertexAttributeDescriptionCount = stride == 0 ? 0 : static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = stride == 0 ? nullptr : attributeDescriptions.data();	//List of vertex attribute descriptions (data format and where to bind to/from)

		// -- INPUT ASSEMBLY --
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		//viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		//viewportStateCreateInfo.pScissors = &scissor;

		// -- DYNAMIC STATES --
		//Dynamic states to enable
		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	//Dynamic viewport : Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	//Dynamic scissor : Can rsize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//Dynamic state creation info
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

		// -- RASTERIZER --
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;		//Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;//Whether to discard data skip rasterizer, only suitable for pipeline without framebuffer output
		rasterizerCreateInfo.polygonMode = polygonMode;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

		// -- MULTISAMPLING --
		VkPipelineMultisampleStateCreateInfo multisamplineCreateInfo = {};
		multisamplineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplineCreateInfo.sampleShadingEnable = VK_FALSE;
		multisamplineCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// -- BLENDING --

		//Blend Attachment State (how blending is handled)
		VkPipelineColorBlendAttachmentState colourState = {};
		colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	//Colours to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourState.blendEnable = VK_TRUE;			//Enable blending

		colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourState.colorBlendOp = VK_BLEND_OP_ADD;	

		//Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//			  (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)
		
		colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourState.alphaBlendOp = VK_BLEND_OP_ADD;
		//Summarized: (1 * new alpha) + (0 * old colour) = new alpha

		VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
		colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlendingCreateInfo.logicOpEnable = VK_FALSE;	//Alternative to calculations is to use logical operations
		colourBlendingCreateInfo.attachmentCount = 1;
		colourBlendingCreateInfo.pAttachments = &colourState;

		// -- PIPELINE LAYOUT --

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

		VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// -- DEPTH STENCIL TESTING --
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = depthWriteEnable;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;	//Does depth value exists between bounds
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.front = {};
		depthStencilCreateInfo.back = {};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisamplineCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineCreateInfo.layout = mPipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = subpassIndex;
		//Pipeline derivatives: Can create multiple pipelines that derive from one another for optimisation
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	//Existing pipeline to derive from...
		pipelineCreateInfo.basePipelineIndex = -1;	//or index of pipeline being created to derive from (in case creating multiple at once)

		//Create graphics pipeline
		result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a graphics pipeline!");
		}

		//Destroy shader modules, no longer neede after Pipeline created
		std::for_each(shaders.begin(), shaders.end(), [logicalDevice](VulkanShader& shader)
			{shader.destroy(logicalDevice);}
		);
    }

	void VulkanPipeline::destroy(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
	}
}