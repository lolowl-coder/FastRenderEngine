#pragma once

#define NOMINMAX
#include <windows.h>

#include "fre/core/IWindow.hpp"

namespace fre
{
    class ExternalWindowWindows : public IWindow
    {
    public:
        explicit ExternalWindowWindows(HWND hwnd)
            : m_hwnd(hwnd) {
        }

        NativeWindowHandle getNativeHandle() const override
        {
            return { m_hwnd, nullptr };
        }

    private:
        HWND m_hwnd;
    };
}