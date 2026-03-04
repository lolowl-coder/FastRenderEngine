#pragma once

namespace fre
{
    struct NativeWindowHandle
    {
        // HWND on Windows
        void* handle = nullptr;
        // HINSTANCE on Windows (if needed)
        void* instance = nullptr;
    };
}