#include "fre/core/IScene.hpp"
#include "fre/core/IWindow.hpp"
#include "fre/core/FileSystem.hpp"
#include "fre/core/Log.hpp"
#include "fre/core/PlatformFactory.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/core/ThreadPool.hpp"
#include "fre/core/VirtualFileSystem.hpp"
#include "fre/core/WindowManager.hpp"
#include "fre/engine/Engine.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGraphicsContext.hpp"
#include "fre/renderer/IRenderer.hpp"
#include "fre/renderer/IShader.hpp"
#include "fre/renderer/ISurface.hpp"
#include "fre/renderer/RendererFactory.hpp"
#include "fre/renderer/RenderPassData.hpp"
#include "fre/renderer/ShaderLoadDesc.hpp"
#include "fre/renderer/ShaderStageBlob.hpp"
#include "fre/renderer/ShaderSystem.hpp"

namespace fre
{
	Engine::Engine(IWindow* window, VirtualFileSystem* vfs, ThreadPool& threadPool)
        : mVFS(*vfs)
        , mThreadPool(threadPool)
		, mShaderSystem(std::make_unique<ShaderSystem>())
	{
        loadAssets();

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
        mShaderSystem->setResourceFactory(mRenderer->gpuResourceFactory());
        mShaderSystem->createGpuResources();
        mFullscreenShader = mShaderSystem->getShader("FullscreenTri");
	}

    void Engine::loadAssets()
    {
        loadShaders();
    }

	void Engine::loadShaders()
    {
        std::vector<ShaderLoadDesc> requiredShaders =
        {
            {
                "FullscreenTri",
                {
                    { ShaderStage::Vertex, "FullscreenTri.vert.spv", "main" },
                    { ShaderStage::Fragment, "FullscreenTri.frag.spv", "main" }
                }
            }
        };

        for(auto& desc : requiredShaders)
        {
            /*mThreadPool.enqueue([this, &desc]()
                {
                    for(auto& stage : desc.stages)
                    {
                        ShaderStageBlob blob =
                        {
                            .bytecode = mVFS.readFile(stage.path),
                            .entryPoint = stage.entryPoint
                        };

                        mShaderSystem->registerBlob(desc.name, stage.stage, std::move(blob));
                    }
                }
            );*/
            for(auto& stage : desc.stages)
            {
                ShaderStageBlob blob =
                {
                    .bytecode = mVFS.readFile(stage.path),
                    .entryPoint = stage.entryPoint
                };

                mShaderSystem->registerBlob(desc.name, stage.stage, std::move(blob));
            }
        }
    }

    void Engine::update()
    {
	}

    void Engine::render()
    {
        mRenderer->beginFrame();
		RenderPassData renderPassData = { .shader = mFullscreenShader };
		mRenderer->renderFrame(mScene.get(), renderPassData);
        mRenderer->endFrame();
	}
}