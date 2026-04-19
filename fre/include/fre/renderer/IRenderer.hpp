#pragma once

#include "fre/core/IScene.hpp"

namespace fre
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual void beginFrame() = 0;
		virtual void renderFrame(IScene* scene) = 0;
		virtual void endFrame() = 0;
	};
}