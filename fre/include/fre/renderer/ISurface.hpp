#pragma once

#include "fre/core/Format.hpp"

#include <cstdint>

namespace fre
{
    struct Frame;

    class ISurface {
    public:
        struct Config {
            uint32_t mWidth;
            uint32_t mHeight;
            Format mColorFormat;
            Format mDepthFormat;
            uint32_t mBufferCount;
            bool mVSync;
        };

    public:
        virtual ~ISurface() = default;

        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
    };
}