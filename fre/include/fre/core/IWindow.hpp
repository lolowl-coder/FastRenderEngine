#pragma once

#include "fre/core/NativeWindowHandle.hpp"

#include <stdint.h>

namespace fre
{
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        virtual NativeWindowHandle getNativeHandle() const = 0;
        virtual uint32_t width() const = 0;
        virtual uint32_t height() const = 0;
    };
}