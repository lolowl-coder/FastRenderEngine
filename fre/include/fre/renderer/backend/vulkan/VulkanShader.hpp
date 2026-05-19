#pragma once

#include "fre/renderer/backend/vulkan/EnumConvert.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/IShader.hpp"
#include "fre/renderer/ShaderStage.hpp"
#include "fre/renderer/ShaderStageBlob.hpp"

#include <vector>
#include <unordered_map>

namespace fre
{
    class VulkanShader : public IShader
    {
    public:
        VulkanShader(const std::string& name, std::unordered_map<ShaderStage, ShaderStageBlob>& blobs, vk::Device logicalDevice)
			: mName(name)
        {
			for(auto& [stage, blob] : blobs)
            {
                // Create shader modules from blobs
                vk::ShaderModuleCreateInfo shaderModuleCreateInfo(
                    vk::ShaderModuleCreateFlags(),
                    blob.bytecode.size(),
                    reinterpret_cast<const uint32_t*>(blob.bytecode.data()));
                auto module = vkCheck(logicalDevice.createShaderModule(shaderModuleCreateInfo));
				auto vkStage = toVk(stage);
                mStages[vkStage] = { module, vkStage };
            }
		}
        struct Stage
        {
            vk::ShaderModule module;
            vk::ShaderStageFlagBits stage;
        };

		virtual const std::string& getName() const override { return mName; }
        const std::unordered_map<vk::ShaderStageFlagBits, Stage>& getStages() const { return mStages; }

        vk::PipelineLayout getPipelineLayout() const { return mPipelineLayout; }

    private:
		std::string mName;
        std::unordered_map<vk::ShaderStageFlagBits, Stage> mStages;

        vk::PipelineLayout mPipelineLayout = VK_NULL_HANDLE;

        // future:
        // reflection data
        // descriptor set layouts
    };
}