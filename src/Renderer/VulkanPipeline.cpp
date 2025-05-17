#include "Renderer/VulkanPipeline.hpp"

#include "Renderer/VulkanShader.hpp"

#include <algorithm>
#include <stdexcept>

namespace fre
{
	std::vector<VkPipelineShaderStageCreateInfo> getPipelineShaderStageCreateInfo(const std::vector<VulkanShader*> shaders)
	{
		std::vector<VkPipelineShaderStageCreateInfo> result;
		for(auto shader : shaders)
		{
			VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			//Shader stage name
			shaderStageCreateInfo.stage = shader->mShaderStage;
			//Shader module to be used by stage
			shaderStageCreateInfo.module = shader->mShaderModule;
			//Entry point in to shader
			shaderStageCreateInfo.pName = "main";

			result.push_back(shaderStageCreateInfo);
		}

		return result;
	}

	VkPipelineLayout createPipelineLayout(
		VkDevice logicalDevice,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstantRanges)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VK_CHECK(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		return pipelineLayout;
	}

    void VulkanPipeline::createGeometryPipeline(
		VkDevice logicalDevice,
		std::vector<VulkanShader*> shaders,
		VkPrimitiveTopology topology,
		uint32_t stride,
        const std::vector<VulkanVertexAttribute>& vertexAttributes,
		VkBool32 depthWriteEnable,
		VkRenderPass renderPass,
		uint32_t subpassIndex,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstantRanges,
		uint32_t attachmentsCount,
		float lineWidth,
		VkCullModeFlags cullMode)
    {
        //Put shader stage creation info in to container
		//Graphics Pipeline creation info requires array of shader stage creates
        auto shaderStages = getPipelineShaderStageCreateInfo(shaders);
        
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
		inputAssembly.topology = topology;
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
		if(lineWidth > 0.0f)
		{
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
		}

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
		rasterizerCreateInfo.polygonMode = topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST ? VK_POLYGON_MODE_POINT : VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = std::max(1.0f, lineWidth);
		rasterizerCreateInfo.cullMode = cullMode;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

		// -- MULTISAMPLING --
		VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
		multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
		multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		// Optional
		//multisamplingCreateInfo.minSampleShading = 1.0f;
		//multisamplingCreateInfo.pSampleMask = nullptr;
		//multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
		//multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

		// -- BLENDING --

		//Blend Attachment State (how blending is handled)
		VkPipelineColorBlendAttachmentState colorState = {};
		colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	//Colours to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorState.blendEnable = VK_TRUE;			//Enable blending

		colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorState.colorBlendOp = VK_BLEND_OP_ADD;	

		//Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//			  (new color alpha * new colour) + ((1 - new colour alpha) * old colour)
		
		colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorState.alphaBlendOp = VK_BLEND_OP_ADD;
		//Summarized: (1 * new alpha) + (0 * old color) = new alpha

		std::vector<VkPipelineColorBlendAttachmentState> colorStates(attachmentsCount, colorState);

		VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
		colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCreateInfo.logicOpEnable = VK_FALSE;	//Alternative to calculations is to use logical operations
		colorBlendingCreateInfo.attachmentCount = colorStates.size();
		colorBlendingCreateInfo.pAttachments = colorStates.data();

		// -- PIPELINE LAYOUT --

		mPipelineLayout = createPipelineLayout(logicalDevice, descriptorSetLayouts, pushConstantRanges);

		// -- DEPTH STENCIL TESTING --
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = depthWriteEnable;
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
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineCreateInfo.layout = mPipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = subpassIndex;
		//Pipeline derivatives: Can create multiple pipelines that derive from one another for optimisation
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	//Existing pipeline to derive from...
		pipelineCreateInfo.basePipelineIndex = -1;	//or index of pipeline being created to derive from (in case creating multiple at once)

		mBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		//Create graphics pipeline
		VK_CHECK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline));
    }

	void VulkanPipeline::createComputePipeline(
        VkDevice logicalDevice,
        VulkanShader& shader,
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstantRanges)
	{
		mPipelineLayout = createPipelineLayout(logicalDevice, descriptorSetLayouts, pushConstantRanges);

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = mPipelineLayout;
        auto shaderStageInfos = getPipelineShaderStageCreateInfo({&shader});
		pipelineInfo.stage = shaderStageInfos.front();

		VK_CHECK(vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline));

        mBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

		shader.destroy(logicalDevice);
	}

	void VulkanPipeline::createShaderBindingTables(
		MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& mRayTracingPipelineProperties,
		VulkanBufferManager& bufferManager)
	{
		const uint32_t           handle_size = mRayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t           handle_size_aligned = alignedSize(mRayTracingPipelineProperties.shaderGroupHandleSize, mRayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t           handle_alignment = mRayTracingPipelineProperties.shaderGroupHandleAlignment;
		const uint32_t           group_count = static_cast<uint32_t>(mShaderGroups.size());
		const uint32_t           sbt_size = group_count * handle_size_aligned;
		const VkBufferUsageFlags sbt_buffer_usage_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags     sbt_memory_usage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		// Copy the pipeline's shader handles into a host buffer
		std::vector<uint8_t> shader_handle_storage(sbt_size);
		VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(mainDevice.logicalDevice, mPipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

		// Raygen
		// Create binding table buffers for each shader type
		mRaygenShaderBindingTable = bufferManager.createBuffer(mainDevice, transferQueue, transferCommandPool,
			sbt_buffer_usage_flags, sbt_memory_usage, shader_handle_storage.data(), handle_size);
		mMissShaderBindingTable = bufferManager.createBuffer(mainDevice, transferQueue, transferCommandPool,
			sbt_buffer_usage_flags, sbt_memory_usage, shader_handle_storage.data() + handle_size_aligned, handle_size);
		mHhitShaderBindingTable = bufferManager.createBuffer(mainDevice, transferQueue, transferCommandPool,
			sbt_buffer_usage_flags, sbt_memory_usage, shader_handle_storage.data() + handle_size_aligned * 2, handle_size);
	}

	void VulkanPipeline::createRTPipeline(VkDevice logicalDevice,
		std::vector<VulkanShader*> shaders,
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstantRanges)
	{
		mPipelineLayout = createPipelineLayout(logicalDevice, descriptorSetLayouts, pushConstantRanges);
		
		auto shaderStageInfos = getPipelineShaderStageCreateInfo(shaders);

		for(uint32_t i = 0; i < shaderStageInfos.size(); i++)
		{
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			if(shaders[i]->mShaderStage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
			{
				shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
				shaderGroup.closestHitShader = i;
				shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
				shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			}
			else
			{
				shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				shaderGroup.generalShader = i;
				shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
				shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
				shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			}
			mShaderGroups.push_back(shaderGroup);
		}

		VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
		raytracing_pipeline_create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		raytracing_pipeline_create_info.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
		raytracing_pipeline_create_info.pStages = shaderStageInfos.data();
		raytracing_pipeline_create_info.groupCount = static_cast<uint32_t>(mShaderGroups.size());
		raytracing_pipeline_create_info.pGroups = mShaderGroups.data();
		raytracing_pipeline_create_info.maxPipelineRayRecursionDepth = 1;
		raytracing_pipeline_create_info.layout = mPipelineLayout;
		VK_CHECK(vkCreateRayTracingPipelinesKHR(logicalDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &mPipeline));
		
		mBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	}

	void VulkanPipeline::destroy(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, mPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, mPipelineLayout, nullptr);
	}

	bool VulkanPipeline::isCompute() const
	{
		return mBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE;
	}
}