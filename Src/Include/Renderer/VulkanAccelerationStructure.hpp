#pragma once

namespace fre
{
	// Wraps all data required for an acceleration structure
	struct AccelerationStructure
	{
		VkAccelerationStructureKHR handle;
		uint64_t device_address;
		VulkanBuffer buffer;
	};
}