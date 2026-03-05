#pragma once

#include "RendererDesc.hpp"

namespace fre
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual bool initialize(const RendererDesc& desc) = 0;
		virtual void shutdown() = 0;

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual void waitIdle() = 0;
	};
}