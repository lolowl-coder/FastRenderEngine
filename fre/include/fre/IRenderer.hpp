#pragma once

#include "RendererConfig.hpp"

namespace fre
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual bool initialize(const RendererConfig& desc) = 0;
		virtual void shutdown() = 0;

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		virtual void waitIdle() = 0;
	};
}