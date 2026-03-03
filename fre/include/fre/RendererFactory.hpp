#pragma once
#include <memory>

namespace fre
{
	enum class RenderAPI
	{
		Vulkan
	};

	class IRenderer;

	std::unique_ptr<IRenderer> createRenderer(RenderAPI api);
}