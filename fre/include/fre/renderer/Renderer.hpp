#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/IRenderer.hpp"
#include "fre/renderer/backend/IRenderBackend.hpp"

namespace fre
{
	class Renderer : public IRenderer
	{
	public:
		Renderer(const CommonRendererConfig& commonConfig);
		virtual void beginFrame() override;
		virtual void renderFrame(IScene* scene, RenderPassData& renderPassData) override;
		virtual void endFrame() override;
		virtual IGpuResourceFactory* gpuResourceFactory() override { return dynamic_cast<IGpuResourceFactory*>(mBackend.get()); }
	private:
		CommonRendererConfig mCommonConfig;
		RenderBackendPtr mBackend;
	};
}