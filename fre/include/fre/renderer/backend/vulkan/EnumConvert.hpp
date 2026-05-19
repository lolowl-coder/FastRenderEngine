#pragma once

#include "fre/renderer/backend/Vulkan/VulkanCommon.hpp"
#include "fre/renderer/ShaderStage.hpp"

namespace fre
{
	inline vk::ShaderStageFlagBits toVk(ShaderStage stage)
	{
        switch(stage)
        {
			case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
			case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
			case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
        };
	}
}