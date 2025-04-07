#pragma once

#include "Renderer/VulkanRenderer.hpp"
#include "ThreadPool.hpp"

namespace app
{
	class AppRenderer : public fre::VulkanRenderer
	{
	public:
		AppRenderer(fre::ThreadPool& threadPool)
			: VulkanRenderer(threadPool)
		{
		}
	protected:
		virtual void requestExtensions() override;
		virtual void requestDeviceFeatures() override;
	};
}