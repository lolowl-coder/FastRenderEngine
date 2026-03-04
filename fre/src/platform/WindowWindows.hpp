#pragma once

#define NOMINMAX
#include <windows.h>

namespace fre
{
    class WindowWindows : public IWindow
    {
    public:
        NativeWindowHandle getNativeHandle() const override
        {
            return { m_hwnd, m_hInstance };
        }

        uint32_t width() const override
        {
            RECT rect{};
            GetClientRect(m_hwnd, &rect);
            return static_cast<uint32_t>(rect.right - rect.left);
        }

        uint32_t height() const override
        {
            RECT rect{};
            GetClientRect(m_hwnd, &rect);
            return static_cast<uint32_t>(rect.bottom - rect.top);
        }

    private:
        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
    };
}