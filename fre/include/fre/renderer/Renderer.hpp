#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IRenderer.hpp"

namespace fre
{
	class Renderer : public IRenderer
	{
	public:
		Renderer(const CommonRendererConfig& commonConfig);
		virtual void beginFrame() override;
		virtual void renderFrame(IScene* scene) override;
		virtual void endFrame() override;
	private:
		CommonRendererConfig mCommonConfig;
		RenderBackendPtr mBackend;
	};
}