#pragma once

#include "fre/core/Pointers.hpp"

namespace fre
{
	class Engine
	{
	public:
		Engine(IWindow* window);
		virtual ~Engine() = default;
		void update();
		void render();
	private:
		GraphicsContextPtr mGraphicsContext;
		SurfacePtr mSurface;
		RendererPtr mRenderer;
		ScenePtr mScene;
	};
}