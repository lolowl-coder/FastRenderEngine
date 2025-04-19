#pragma once

#include "Renderer/VulkanBufferManager.hpp"
#include <volk.h>

namespace fre
{
	// Wraps all data required for an acceleration structure
	struct AccelerationStructure
	{
		VkAccelerationStructureKHR mHandle;
		uint64_t mDeviceAddress;
		VulkanBuffer mBuffer;
	};
}