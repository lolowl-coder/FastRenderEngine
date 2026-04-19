#pragma once

#include "fre/core/Pointers.hpp"

namespace fre
{
	class Application
	{
	public:
		Application();
		virtual ~Application();
		void mainLoop();
	private:
		void handleInput();
	private:
		WindowPtr mWindow;
		EnginePtr mEngine;
	};
}