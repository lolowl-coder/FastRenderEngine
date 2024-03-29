#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/VulkanQueueFamily.hpp"

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

namespace fre
{
	const int MAX_FRAME_DRAWS = 3;
	const int MAX_OBJECTS = 40;

	struct MainDevice
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	};

	const std::vector<const char*> deviceExtentions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//Vertex data representation
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 tex;
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<char> readFile(const std::string& fileName);

	uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);

	std::vector<VulkanQueueFamily> getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	template<typename T>
	int getIndexOf(const std::vector<T>& v, const T& value)
	{
		int result = -1;
		const auto& foundIt = std::find(v.begin(), v.end(), value);
		if(foundIt != v.end())
		{
			result = static_cast<int>(std::distance(v.begin(), foundIt));
		}

		return result;
	}

	template<typename T>
	bool areEqual(T a, T b, T epsilon = std::numeric_limits<T>::epsilon())
	{
		return std::abs(a - b) < epsilon;
	}
}