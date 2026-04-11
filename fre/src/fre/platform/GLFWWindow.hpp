#pragma once

#define GLFW_EXPOSE_NATIVE_WIN32

#include <windows.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "fre/core/IWindow.hpp"
#include "fre/platform/IWin32Window.hpp"
#include "fre/platform/IGLFWWindow.hpp"

namespace fre
{
    class GLFWWindow : public IWin32Window, public IGLFWWindow
    {
    public:
        GLFWWindow(const IWindow::Desc& desc);
        virtual std::vector<RequirementRequest> getInstanceExtensions() override;
		virtual void setPosition(const int width, const int height) override;
        virtual void onSizeChanged(const int width, const int height) override;
        virtual void onCustomMessage(const uint32_t messageId) override;
        virtual void pollEvents() override;
        virtual HWND getHWND() const override { return mHwnd;  }
        virtual GLFWwindow* getGLFWwindow() const override { return mWindow;  }
        virtual uint32_t width() const override;
        virtual uint32_t height() const override;
        virtual bool shouldClose() const override;
		WNDPROC getOriginalWndProc() const { return mOriginalWndProc; }
    private:
        HWND mHwnd = nullptr;
        HINSTANCE mHInstance = nullptr;
        WNDPROC mOriginalWndProc;
        GLFWwindow* mWindow;
    };
}