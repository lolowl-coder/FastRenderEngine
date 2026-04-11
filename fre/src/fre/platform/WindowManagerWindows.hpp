#include "fre/core/WindowManager.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace fre
{
	class WindowManagerWindows : public IWindowManager
	{
	public:
		WindowManagerWindows();

		~WindowManagerWindows();

		virtual WindowPtr createWindow(const IWindow::Desc& desc) override;
	};
}