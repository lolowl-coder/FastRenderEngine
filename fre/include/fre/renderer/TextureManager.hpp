#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/TextureHandle.hpp"

namespace fre
{
    class TextureManager
    {
    public:
        TextureManager(IGpuResourceFactory& gpuResourceFactory);
        TextureHandle create(const IGpuImage::Desc& imageDesc, const IGpuImageView::Desc& imageViewDesc);
        void destroy(TextureHandle handle);
        TexturePtr& get(TextureHandle handle);

    private:
        IGpuResourceFactory& mGpuResourceFactory;
        std::vector<GpuImagePtr> mImages;
        std::vector<GpuImageViewPtr> mImageViews;
        std::vector<TexturePtr> mTextures;
    };
}