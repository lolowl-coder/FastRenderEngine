#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"
#include "fre/renderer/ShaderStage.hpp"
#include "fre/renderer/ShaderStageBlob.hpp"

#include <unordered_map>

namespace fre
{
	class IGpuResourceFactory
	{
	public:
		virtual ~IGpuResourceFactory() = default;
		virtual GpuImagePtr createGpuImage(const IGpuImage::Desc& desc) = 0;
		virtual GpuImageViewPtr createGpuImageView(IGpuImage* image, const IGpuImageView::Desc& desc) = 0;
		virtual ShaderPtr createGpuShader(const std::string& name, std::unordered_map<ShaderStage, ShaderStageBlob>& blobs) = 0;
	};
}