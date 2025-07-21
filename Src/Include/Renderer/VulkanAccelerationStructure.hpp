#pragma once

#include "Renderer/VulkanBufferManager.hpp"
#include <volk.h>

namespace fre
{
	// Wraps all data required for an acceleration structure
	struct AccelerationStructure
	{
		VkAccelerationStructureKHR mHandle = VK_NULL_HANDLE;
		uint64_t mDeviceAddress = 0;
		VulkanBuffer mBuffer;
	};
}