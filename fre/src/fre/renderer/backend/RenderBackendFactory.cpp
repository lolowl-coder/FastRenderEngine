#include "fre/renderer/backend/vulkan/VulkanRenderBackend.hpp"

namespace fre
{
	RenderBackendPtr createRenderBackend(const CommonRendererConfig& commonConfig)
	{
		switch (commonConfig.mBackend)
		{
			case Backend::Vulkan: return std::make_unique<VulkanRenderBackend>(commonConfig);
		}
		return nullptr;
	}
}