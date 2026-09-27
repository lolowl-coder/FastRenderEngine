#pragma once

#include "fre/renderer/IGPUImageView.hpp"
#include "fre/renderer/RendererStructs.hpp"
#include "fre/renderer/PSO.hpp"
#include <glm/glm.hpp>

namespace fre
{
	struct RenderPassData
	{
		IShader* shader = nullptr;
		RenderTargetState renderTargetState;
	};

	struct RenderPassContext
	{
		Extent2D extent;
		std::vector<IGpuImageView*> attachments;
		glm::vec4 clearColor;
	};
}