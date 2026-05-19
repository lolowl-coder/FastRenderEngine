#pragma once

#include "fre/core/IScene.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/RenderPassData.hpp"

namespace fre
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual IGpuResourceFactory* gpuResourceFactory() = 0;

		virtual void beginFrame() = 0;
		virtual void renderFrame(IScene* scene, RenderPassData& renderPassData) = 0;
		virtual void endFrame() = 0;
	};
}