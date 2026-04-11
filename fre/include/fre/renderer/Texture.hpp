#pragma once

#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"

namespace fre
{
	class Texture
	{
	public:
		Texture(IGpuImage& gpuImage, IGpuImageView& imageView);
		virtual uint32_t width() const;
		virtual uint32_t height() const;
		virtual Format format() const;
	private:
		IGpuImage& mGpuImage;
		IGpuImageView& mGpuImageView;
	};
}

