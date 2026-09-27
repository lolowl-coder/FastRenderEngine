#pragma once

#include "fre/core/Format.hpp"
#include "fre/renderer/backend/Vulkan/VulkanCommon.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"
#include "fre/renderer/SampleCount.hpp"
#include "fre/renderer/RendererStructs.hpp"
#include "fre/renderer/ShaderStage.hpp"

namespace fre
{
	vk::Format toVk(fre::Format format);
	fre::Format fromVk(vk::Format format);
	vk::ShaderStageFlagBits toVk(ShaderStage stage);
	vk::ImageUsageFlags toVk(ImageUsage usage);
	vk::ComponentSwizzle toVk(ComponentSwizzle s);
	vk::ImageAspectFlagBits toVk(Aspect aspect);
	vk::SampleCountFlagBits toVk(SampleCount count);
	vk::Viewport toVk(const Viewport& viewport);
	vk::Rect2D toVk(const ScissorRect& scissor);
}