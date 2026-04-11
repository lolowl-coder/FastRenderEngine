#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"

namespace fre
{
	GraphicsContextPtr createGraphicsContext(const Backend backend, IWindow* window, const bool enableValidation);
	RendererPtr createRenderer(const CommonRendererConfig& commonConfig);
}