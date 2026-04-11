#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/core/IWindow.hpp"

namespace fre
{
	class IWindowManager
	{
	public:
		IWindowManager() {};
		~IWindowManager() = default;
		virtual WindowPtr createWindow(const IWindow::Desc& windowDesc) = 0;
	};
}