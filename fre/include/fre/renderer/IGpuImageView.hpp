#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/core/Format.hpp"
#include "fre/renderer/IGpuImage.hpp"

namespace fre
{
    enum class ComponentSwizzle
    {
        // Use original
        Identity,
        Zero,
        One,
        R,
        G,
        B,
        A
    };

    struct ComponentMapping
    {
        ComponentSwizzle r = ComponentSwizzle::Identity;
        ComponentSwizzle g = ComponentSwizzle::Identity;
        ComponentSwizzle b = ComponentSwizzle::Identity;
        ComponentSwizzle a = ComponentSwizzle::Identity;
    };

    vk::ComponentSwizzle toVk(ComponentSwizzle s);

    enum class Aspect
    {
        Color,
        Depth,
        Stencil,
        Metadata,
        Plane0,
        Plane0KHR,
        Plane1,
        Plane1KHR,
        Plane2,
        Plane2KHR,
        None,
        NoneKHR,
        MemoryPlane0EXT,
        MemoryPlane1EXT,
        MemoryPlane2EXT,
        MemoryPlane3EXT
    };

    vk::ImageAspectFlagBits toVk(Aspect aspect);

    class IGpuImageView
    {
    public:
        struct Desc
        {
            Format mFormat = Format::RGBA8_UNorm;
            uint32_t mBaseMip = 0;
            uint32_t mMipCount = 1;
            uint32_t mBaseLayer = 0;
            uint32_t mLayerCount = 1;
            Aspect mAspect = Aspect::Color;
            ComponentMapping mComponentMapping = {};
        };
		IGpuImageView() = default;
        virtual ~IGpuImageView() noexcept = default;
        IGpuImageView(const IGpuImageView&) = delete;
        IGpuImageView& operator=(const IGpuImageView&) = delete;

        IGpuImageView(IGpuImageView&&) noexcept = default;
        IGpuImageView& operator=(IGpuImageView&&) noexcept = default;

        virtual uint32_t baseMip() const = 0;
        virtual uint32_t mipCount() const = 0;

        virtual uint32_t baseLayer() const = 0;
        virtual uint32_t layerCount() const = 0;

        virtual ComponentMapping componentMapping() const = 0;
    };
}