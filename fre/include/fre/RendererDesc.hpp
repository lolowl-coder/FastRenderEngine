#pragma once
#include <cstdint>

namespace fre
{
	struct RendererDesc
	{
		void* windowHandle = nullptr;  // HWND
		uint32_t width = 0;
		uint32_t height = 0;
		bool headless = false;
		bool enableValidation = false;
	};
}