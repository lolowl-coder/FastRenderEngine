#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"

namespace fre
{
	class IGpuResourceFactory
	{
	public:
		virtual ~IGpuResourceFactory() = default;
		virtual GpuImagePtr createGpuImage(const IGpuImage::Desc& desc) = 0;
		virtual GpuImageViewPtr createGpuImageView(IGpuImage* image, const IGpuImageView::Desc& desc) = 0;
	};
}