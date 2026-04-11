#pragma once

#include <windows.h>

namespace fre
{
    class IWin32Window : public IWindow
    {
    public:
        virtual HWND getHWND() const = 0;
    };
}