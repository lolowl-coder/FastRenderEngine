#pragma once

#include "fre/core/Format.hpp"

#include <glm/glm.hpp>

namespace fre
{

    enum class ImageUsage : uint32_t
    {
        None = 0,
        Color = 1 << 0,
        DepthStencil = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5,
        Input = 1 << 6,
        Transient = 1 << 7
    };

    inline ImageUsage operator | (ImageUsage a, ImageUsage b)
    {
        return static_cast<ImageUsage>(uint32_t(a) | uint32_t(b));
    }

    inline ImageUsage operator & (ImageUsage a, ImageUsage b)
    {
        return static_cast<ImageUsage>(uint32_t(a) & uint32_t(b));
    }

    /*inline ImageUsage& operator |= (ImageUsage& a, ImageUsage b)
    {
        a = a | b;
        return a;
    }*/

    vk::ImageUsageFlags toVk(ImageUsage usage);

    class IGpuImage
    {
    public:
        struct Desc
        {
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t depth = 1;
            Format format = Format::RGBA8_UNorm;
            ImageUsage usage = ImageUsage::Sampled;
            uint32_t mipLevels;
            uint32_t layers;
        };

        virtual ~IGpuImage() noexcept = default;

        virtual Format format() const = 0;
        virtual glm::ivec3 dimensions() const = 0;

        virtual uint32_t mipLevels() const = 0;
        virtual uint32_t layers() const = 0;
    };
}