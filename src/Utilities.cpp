#include "Shader.hpp"
#include "Utilities.hpp"

namespace fre
{
    std::vector<char> readFile(const std::string& fileName)
	{
		//Open stream from given file
		std::ifstream file(fileName, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file! " + fileName);
		}

		const auto fileSize = (size_t)file.tellg();
		file.seekg(0);
		std::vector<char> fileBuffer(fileSize);
		file.read(fileBuffer.data(), fileSize);

		file.close();

		return fileBuffer;
	}

    uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
	{
		//Get properties of physical device memory
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((allowedTypes & (i << 1))	//Index of memory type must match corresponding bit in allowedTypes
				&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)	//Desired property bit flags are part of memory type's property flags
			{
				return i;
			}
		}

		return std::numeric_limits<uint32_t>::max();
	}

	std::vector<VulkanQueueFamily> getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		//Get all Queue Family properties info for the given device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());
		
		std::vector<VulkanQueueFamily> result(queueFamilyCount);
		for(uint8_t i = 0; i < queueFamilyList.size(); i++)
		{
			result[i].init(device, surface, queueFamilyList[i], i);
		}

		return result;
	}
}