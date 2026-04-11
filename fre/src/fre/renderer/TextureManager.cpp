#include "fre/renderer/Texture.hpp"
#include "fre/renderer/TextureManager.hpp"

namespace fre
{
    TextureManager::TextureManager(IGpuResourceFactory& gpuResourceFactory)
        : mGpuResourceFactory(gpuResourceFactory)
    {
    }

    TextureHandle TextureManager::create(const IGpuImage::Desc& imageDesc, const IGpuImageView::Desc& imageViewDesc)
    {
        auto image = mGpuResourceFactory.createGpuImage(imageDesc);
        auto view = mGpuResourceFactory.createGpuImageView(image.get(), imageViewDesc);
		auto texture = std::make_unique<Texture>(*image, *view);

        TextureHandle handle = { .index = static_cast<uint32_t>(mTextures.size()) };

		mImages.push_back(std::move(image));
		mImageViews.push_back(std::move(view));
		mTextures.push_back(std::move(texture));

		return handle;
    }

    void TextureManager::destroy(TextureHandle handle)
    {
		mImages.erase(mImages.begin() + handle.index);
		mImageViews.erase(mImageViews.begin() + handle.index);
		mTextures.erase(mTextures.begin() + handle.index);
    }

    TexturePtr& TextureManager::get(TextureHandle handle)
    {
		assert(handle.index < mTextures.size());

		return mTextures[handle.index];
    }
}