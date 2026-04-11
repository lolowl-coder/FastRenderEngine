#pragma once

#include "fre/core/Pointers.hpp"

namespace fre
{
	class IGraphicsContext
	{
	public:
		virtual ~IGraphicsContext() = default;

		virtual SurfacePtr createSurface(IWindow* window) = 0;
	};
}