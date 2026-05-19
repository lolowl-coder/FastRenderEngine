#pragma once

#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanPipeline.hpp"
#include "fre/renderer/backend/vulkan/VulkanShader.hpp"

#include <functional>
#include <unordered_map>
#include <cstdint>

namespace fre
{
    struct PipelineKey
    {
        vk::Format colorFormat;
        vk::Format depthFormat;

        IShader* shader;

        bool operator == (const PipelineKey& other) const = default;
    };

    struct PipelineKeyHash
    {
        size_t operator()(const PipelineKey& k) const
        {
            size_t h = 0;

            auto hashCombine = [&](size_t v)
                {
                    h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2);
                };

            VulkanShader* vkShader = static_cast<VulkanShader*>(k.shader);

            hashCombine(std::hash<uint64_t>()((uint64_t)k.shader));
            hashCombine(std::hash<int>()((int)k.colorFormat));
            hashCombine(std::hash<int>()((int)k.depthFormat));

            return h;
        }
    };

    struct Pipeline
    {
        vk::Pipeline handle;
        vk::PipelineLayout layout;
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
        }

        Pipeline* getOrCreate(const PipelineKey& key)
        {
            auto it = mPipelines.find(key);
            if(it != mPipelines.end())
                return &it->second;

            Pipeline pipeline = createPipeline(key);

            auto [iter, _] = mPipelines.emplace(key, pipeline);
            return &iter->second;
        }
    private:
        Pipeline createPipeline(const PipelineKey& key)
        {
            vk::PipelineRenderingCreateInfo renderingInfo{};
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachmentFormats = &key.colorFormat;
            renderingInfo.depthAttachmentFormat = key.depthFormat;

            vk::GraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.pNext = &renderingInfo;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            
			auto* vkShader = static_cast<VulkanShader*>(key.shader);
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

            auto pipelines = vkCheck(mDevice.createGraphicsPipelines(mVkPipelineCache, pipelineInfo));

			return Pipeline{ pipelines[0], VK_NULL_HANDLE };

            /*mDevice.getPipelineCacheData();
            mDevice.createPipelineCache(, mAllocator);*/
        }

    private:
        std::unordered_map<PipelineKey, Pipeline, PipelineKeyHash> mPipelines;
		vk::Device mDevice;
        VulkanAllocator* mAllocator;
		vk::PipelineCache mVkPipelineCache;
    };
}