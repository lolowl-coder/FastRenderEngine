#include "fre/core/IScene.hpp"
#include "fre/renderer/Renderer.hpp"
#include "fre/renderer/backend/IRenderBackend.hpp"
#include "fre/renderer/backend/RenderBackendFactory.hpp"

namespace fre
{
	Renderer::Renderer(const CommonRendererConfig& commonConfig)
		: mCommonConfig(commonConfig)
	{
		mBackend = createRenderBackend(mCommonConfig);
	}

	void Renderer::beginFrame()
	{
	}

	void Renderer::renderFrame(IScene* scene)
	{
		mBackend->drawFrame(scene);
	}

	void Renderer::endFrame()
	{
	}
}