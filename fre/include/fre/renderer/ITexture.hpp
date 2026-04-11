#pragma once

#include <vulkan/vulkan.hpp>
#include "fre/core/Format.hpp"

#include <cstdint>

namespace fre
{
    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;

        virtual Format format() const = 0;
    };
}