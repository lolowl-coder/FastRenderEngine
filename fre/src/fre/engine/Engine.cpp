#include "fre/core/IScene.hpp"
#include "fre/core/IWindow.hpp"
#include "fre/core/FileSystem.hpp"
#include "fre/core/Log.hpp"
#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/core/VirtualFileSystem.hpp"
#include "fre/core/WindowManager.hpp"
#include "fre/engine/Engine.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGraphicsContext.hpp"
#include "fre/renderer/IRenderer.hpp"
#include "fre/renderer/RendererFactory.hpp"
#include "fre/renderer/ISurface.hpp"

namespace fre
{
	Engine::Engine(IWindow* window)
	{
        CommonRendererConfig commonConfig;
        commonConfig.mEnableValidation = true;
        commonConfig.mHeadless = false;
        commonConfig.mGPUSelectionMode = GPUSelectionMode::Auto;
        commonConfig.mWidth = window->width();
        commonConfig.mHeight = window->height();
        commonConfig.mBackend = Backend::Vulkan;

        mGraphicsContext = createGraphicsContext(commonConfig.mBackend, window, commonConfig.mEnableValidation);
        mSurface = mGraphicsContext->createSurface(window);

        commonConfig.mContext = mGraphicsContext.get();
        commonConfig.mSurface = mSurface.get();

        mRenderer = createRenderer(commonConfig);
	}

    void Engine::update()
    {
	}

    void Engine::render()
    {
        mRenderer->beginFrame();
		mRenderer->renderFrame(mScene.get());
        mRenderer->endFrame();
	}
}