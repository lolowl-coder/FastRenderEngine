#pragma once

#include "fre/core/Pointers.hpp"

namespace fre
{
	class ThreadPool;

	class Engine
	{
	public:
		Engine(IWindow* window, VirtualFileSystem* vfs, ThreadPool& threadPool);
		virtual ~Engine() = default;
		void update();
		void render();
	private:
		void loadAssets();
		void loadShaders();
	private:
		GraphicsContextPtr mGraphicsContext;
		SurfacePtr mSurface;
		RendererPtr mRenderer;
		ScenePtr mScene;
		ShaderSystemPtr mShaderSystem;
		IShader* mFullscreenShader;
		VirtualFileSystem& mVFS;
		ThreadPool& mThreadPool;
	};
}