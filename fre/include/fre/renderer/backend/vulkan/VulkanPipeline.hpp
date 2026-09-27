#pragma once

#include "fre/renderer/PSO.hpp"
#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanPipeline.hpp"
#include "fre/renderer/backend/vulkan/VulkanShader.hpp"

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace fre
{
    struct Pipeline
    {
        vk::Pipeline handle;
        vk::PipelineLayout layout;
    };

    enum class PipelineType : uint8_t
    {
        Graphics,
        Compute,
        RayTracing
    };

    struct PipelineKey
    {
        PipelineType type;
        size_t descHash;
    };

    struct PipelineKeyHash
    {
        size_t operator()(const PipelineKey& k) const noexcept
        {
            return k.descHash ^ (static_cast<size_t>(k.type) << 1);
        }
    };

    struct PipelineKeyEq
    {
        bool operator()(const PipelineKey& a, const PipelineKey& b) const noexcept
        {
            return a.type == b.type && a.descHash == b.descHash;
        }
    };

    class VulkanPipelineBuilder
    {
    public:
        VulkanPipelineBuilder(vk::Device device)
            : mDevice(device)
        {
        }

        vk::PipelineVertexInputStateCreateInfo buildVertexInputCreateInfo(const VertexLayout& vertexLayout)
        {
            vk::PipelineVertexInputStateCreateInfo info{};
            // TEMP: empty layout supported
            if(vertexLayout.attributes.empty())
            {
                info.vertexAttributeDescriptionCount = 0;
                info.pVertexAttributeDescriptions = nullptr;
                info.vertexBindingDescriptionCount = 0;
                info.pVertexBindingDescriptions = nullptr;
                return info;
            }

            // later: reflection-driven conversion here
        }

        vk::Pipeline createPipeline(const vk::PipelineCache& pipelineCache, const GraphicsPipelineDesc& desc)
        {
            // Attachments
            vk::PipelineRenderingCreateInfo renderingInfo{};
            renderingInfo.colorAttachmentCount = desc.renderTargets.colorFormats.size();
			std::vector<vk::Format> vkColorFormats;
            for(auto f : desc.renderTargets.colorFormats)
            {
                vkColorFormats.push_back(toVk(f));
            }
            renderingInfo.pColorAttachmentFormats = vkColorFormats.data();
            renderingInfo.depthAttachmentFormat = toVk(desc.renderTargets.depthFormat);

            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.pNext = &renderingInfo;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            
			// Shader stages
			auto* vkShader = static_cast<VulkanShader*>(desc.shader);
            auto& stages = vkShader->getStages();

            std::vector<vk::PipelineShaderStageCreateInfo> vkStages;

            for(auto& s : stages)
            {
                vk::PipelineShaderStageCreateInfo stage{};
                stage.stage = s.second.stage;
                stage.module = s.second.module;
                stage.pName = "main";

                vkStages.push_back(stage);
            }
            pipelineInfo.pStages = vkStages.data();
            pipelineInfo.stageCount = vkStages.size();
            
            // Color blend state
            std::vector<vk::PipelineColorBlendAttachmentState> attachments;
            for(const auto& a : desc.blend.attachments)
            {
                attachments.push_back(
					a.enable ?
                        vk::PipelineColorBlendAttachmentState(
							1u,
                            vk::BlendFactor::eSrc1Alpha, vk::BlendFactor::eOneMinusSrc1Alpha, vk::BlendOp::eAdd,
                            vk::BlendFactor::eSrc1Alpha, vk::BlendFactor::eOneMinusSrc1Alpha, vk::BlendOp::eAdd,
						    vk::ColorComponentFlagBits::eR |
                            vk::ColorComponentFlagBits::eG |
                            vk::ColorComponentFlagBits::eB |
                            vk::ColorComponentFlagBits::eA
                            )
                    : vk::PipelineColorBlendAttachmentState{}
                );
			}

            vk::PipelineColorBlendStateCreateInfo blend{};
            blend.attachmentCount = desc.blend.attachments.size();
            blend.pAttachments = attachments.data();
            pipelineInfo.pColorBlendState = &blend;

            // Vertex input state. Empty for now. Should be generated from desc.VertexLayout.
            auto vertexInput = buildVertexInputCreateInfo(desc.shader->getVertexLayout());

            pipelineInfo.pVertexInputState = &vertexInput;

			// Pipeline layout
			pipelineInfo.layout = vkShader->getPipelineLayout();

            // Multisampling
            vk::PipelineMultisampleStateCreateInfo msaa{};
            msaa.rasterizationSamples = toVk(desc.multisampleState.samplesCount);
			pipelineInfo.pMultisampleState = &msaa;

			// Rasterization
            vk::PipelineRasterizationStateCreateInfo raster{};
            raster.polygonMode = vk::PolygonMode::eFill;
            raster.cullMode = vk::CullModeFlagBits::eBack;
            raster.frontFace = vk::FrontFace::eCounterClockwise;
            raster.lineWidth = 1.0f;
            pipelineInfo.pRasterizationState = &raster;

            // Viewport state
            vk::PipelineViewportStateCreateInfo viewport{};
            viewport.viewportCount = 1;
            viewport.scissorCount = 1;

            pipelineInfo.pViewportState = &viewport;

			// Dynamic state
            vk::DynamicState states[] =
            {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor
            };

            vk::PipelineDynamicStateCreateInfo dynamic{};

            dynamic.dynamicStateCount = 2;
            dynamic.pDynamicStates = states;
            pipelineInfo.pDynamicState = &dynamic;

            // Input assembly
            vk::PipelineInputAssemblyStateCreateInfo ia{};
            ia.topology = vk::PrimitiveTopology::eTriangleList;

            pipelineInfo.pInputAssemblyState = &ia;

            auto pipeline = vkCheck(mDevice.createGraphicsPipeline(pipelineCache, pipelineInfo));
			return pipeline;
        }

		vk::Device mDevice;
    };

    class VulkanPipelineCache
    {
    public:
        VulkanPipelineCache(vk::Device device, VulkanAllocator* allocator)
        {
            mDevice = device;
            mAllocator = allocator;

            vk::PipelineCacheCreateInfo info{};

            mVkPipelineCache = vkCheck(mDevice.createPipelineCache(info));
            /*mDevice.getPipelineCacheData();
            mDevice.createPipelineCache(, mAllocator);*/
        }

        Pipeline getOrCreate(const GraphicsPipelineDesc& desc)
        {
            GraphicsPipelineDescHash hasher;
            auto descHash = hasher(desc);
            auto key = PipelineKey{ PipelineType::Graphics, descHash };

            auto it = mPipelines.find(key);
            if(it != mPipelines.end())
                return it->second;

            Pipeline pipeline = createPipeline(desc);

            auto [iter, _] = mPipelines.emplace(key, pipeline);
            return iter->second;
        }
    private:
        Pipeline createPipeline(const GraphicsPipelineDesc& desc)
        {
            Pipeline pipeline{};
            VulkanPipelineBuilder builder(mDevice);
            pipeline.handle = builder.createPipeline(mVkPipelineCache, desc);
            pipeline.layout = static_cast<VulkanShader*>(desc.shader)->getPipelineLayout();
            return pipeline;
        }
    private:
		vk::Device mDevice;
        VulkanAllocator* mAllocator;
        std::unordered_map<PipelineKey, Pipeline, PipelineKeyHash, PipelineKeyEq> mPipelines;
		vk::PipelineCache mVkPipelineCache;
    };
}