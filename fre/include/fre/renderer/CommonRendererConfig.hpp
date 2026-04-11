#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/backend/Enums.hpp"
#include "fre/renderer/IGraphicsContext.hpp"
#include "fre/renderer/ISurface.hpp"

#include <cstdint>

namespace fre
{
	enum class GPUSelectionMode
	{
		Auto,
		Index
	};

	struct CommonRendererConfig
	{
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
		bool mEnableValidation = false;
		GPUSelectionMode mGPUSelectionMode = GPUSelectionMode::Auto;
		uint32_t mGPUIndex = 0;
		bool mHeadless = false;
		Backend mBackend = Backend::Vulkan;
		IGraphicsContext* mContext;
		ISurface* mSurface;
	};
}