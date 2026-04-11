#include "fre/core/Log.hpp"
#include "fre/platform/GLFWWindow.hpp"

#define WM_FRE_EVENT  (WM_APP + 1)

namespace fre
{
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        GLFWWindow* w = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        w->onSizeChanged(width, height);
    }

    LRESULT CALLBACK customWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        GLFWWindow* w = reinterpret_cast<GLFWWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if(w != nullptr)
        {
            if(uMsg == WM_FRE_EVENT)
            {
                w->onCustomMessage(uMsg);

                // Message handled
                return 0;
            }
            result = CallWindowProc(w->getOriginalWndProc(), hwnd, uMsg, wParam, lParam);
        }

        return result;
    }

    GLFWWindow::GLFWWindow(const IWindow::Desc& desc)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        mWindow = glfwCreateWindow(desc.width, desc.height, desc.title, nullptr, nullptr);

        glfwSetWindowUserPointer(mWindow, this);

        glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);

        setPosition(desc.width, desc.height);

        //glfwSetCursorPos(mWindow, width / 2, height / 2);
        //glfwSetCursorPosCallback(mWindow, mouseCallback);
        //glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
        //glfwSetScrollCallback(mWindow, scrollCallback);
        //glfwSetKeyCallback(mWindow, keyCallback);

        mHwnd = glfwGetWin32Window(mWindow);
        mOriginalWndProc = (WNDPROC)SetWindowLongPtr(mHwnd, GWLP_WNDPROC, (LONG_PTR)customWndProc);
        SetWindowLongPtr(mHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }

    std::vector<RequirementRequest> GLFWWindow::getInstanceExtensions()
    {
        uint32_t glfwExtensionsCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
		std::vector<RequirementRequest> result;
        for(size_t i = 0; i < glfwExtensionsCount; i++)
        {
            result.push_back({ glfwExtensions[i], Requirement::Required, true });
        }

        return result;
    }

    void GLFWWindow::setPosition(const int width, const int height)
    {
        int count;
        int monitorX, monitorY;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[0]);
        glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);
        glfwSetWindowPos(mWindow,
            monitorX + (videoMode->width - width) / 2,
            monitorY + (videoMode->height - height) / 2);
    }

    void GLFWWindow::onSizeChanged(const int width, const int height)
    {
        LOG_TRACE("Window size changed to {}x{}", width, height);
    }

    void GLFWWindow::onCustomMessage(const uint32_t messageId)
    {
		LOG_TRACE("Custom message received: {}", messageId);
    }

    void GLFWWindow::pollEvents()
    {
        glfwPollEvents();
    }

    uint32_t GLFWWindow::width() const
    {
        RECT rect{};
        GetClientRect(mHwnd, &rect);
        return static_cast<uint32_t>(rect.right - rect.left);
    }

    uint32_t GLFWWindow::height() const
    {
        RECT rect{};
        GetClientRect(mHwnd, &rect);
        return static_cast<uint32_t>(rect.bottom - rect.top);
    }

    bool GLFWWindow::shouldClose() const
    {
        return false;
    }
}