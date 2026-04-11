#pragma once

#include "fre/core/Requirement.hpp"
#include "fre/renderer/IGraphicsContext.hpp"
#include "fre/renderer/ISurface.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
	class VulkanContext : public IGraphicsContext
	{
	public:
		struct Desc
		{
			bool mEnableValidation = false;
			std::vector<RequirementRequest> mInstanceExtensions =
			{
				{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, Requirement::Optional, true }
			};
		};

		VulkanContext(const Desc& desc);

		Desc getDesc() const { return mDesc; }
		vk::Instance getInstance() { return mInstance; }
		virtual SurfacePtr createSurface(IWindow* window) override;
	private:
		bool createInstance();
	private:
		Desc mDesc;
		vk::Instance mInstance;
	};
}