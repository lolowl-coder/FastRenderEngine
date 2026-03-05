#pragma once

#include "fre/Feature.hpp"

#include <cstdint>

namespace fre
{
	enum class GPUSelectionMode
	{
		Auto,
		Index
	};

	struct RendererConfig
	{
		void* mWindowHandle = nullptr;  // HWND
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
		bool mHeadless = false;
		bool mEnableValidation = false;
		GPUSelectionMode mGpuSelectionMode = GPUSelectionMode::Auto;
		uint32_t mGpuIndex = 0;
		RendererFeatureConfig mFeatures;
	};
}