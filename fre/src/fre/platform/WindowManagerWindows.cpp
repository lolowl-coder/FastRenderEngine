#include "fre/platform/WindowManagerWindows.hpp"
#include "fre/platform/GLFWWindow.hpp"

namespace fre
{
	WindowManagerWindows::WindowManagerWindows()
	{
		glfwInit();
	}

	WindowManagerWindows::~WindowManagerWindows()
	{
		glfwTerminate();
	}

	WindowPtr WindowManagerWindows::createWindow(const IWindow::Desc& desc)
	{
		return std::make_unique<GLFWWindow>(desc);
	}
}