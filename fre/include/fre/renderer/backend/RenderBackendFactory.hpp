#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"

namespace fre
{
	RenderBackendPtr createRenderBackend(const CommonRendererConfig& commonConfig);
}