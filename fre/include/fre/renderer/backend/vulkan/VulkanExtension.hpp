#pragma once

#include "fre/core/Requirement.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include <vector>

namespace fre
{
	bool isExtensionSupported(std::vector<vk::ExtensionProperties>& actualExtensions, RequirementRequest& extensionToCheck);
}