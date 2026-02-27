#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

namespace fre
{
	struct MainDevice
	{
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
	};
}