#include "fre/renderer/backend/vulkan/VulkanExtension.hpp"

namespace fre
{
	bool isExtensionSupported(std::vector<vk::ExtensionProperties>& actualExtensions, RequirementRequest& extensionToCheck)
	{
		bool result = false;
		for(auto& ext : actualExtensions)
		{
			if(strcmp(ext.extensionName, extensionToCheck.name.c_str()) == 0)
			{
				result = true;
				break;
			}
		}

		return true;
	}
}