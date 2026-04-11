#include "fre/renderer/ITexture.hpp"

namespace fre
{
	Texture::Texture(IGpuImage& gpuImage, IGpuImageView& imageView)
		: mGpuImage(gpuImage), mGpuImageView(imageView)
	{
	}

	uint32_t Texture::width() const
	{
		return mGpuImage.width();
	}

	uint32_t Texture::height() const
	{
		return mGpuImage.height();
	}

	Format Texture::format() const
	{
		return mGpuImage.format();
	}
}