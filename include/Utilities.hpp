#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

namespace fre
{
	const int MAX_FRAME_DRAWS = 3;
	const int MAX_OBJECTS = 30;

	const std::vector<const char*> deviceExtentions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//Vertex data representation
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 col;
		glm::vec2 tex;
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	//Indices (locations) of queue families if they exist at all\

	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;	//Location of Graphics Queue Family
		int presentationFamily = -1;//Location of Presentation Queue Family

		//Check if Queue Families are valid
		bool isValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0;
		}
	};

	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;		//Surface properties, e.g. image size/extent
		std::vector<VkSurfaceFormatKHR> formats;			//Surface image formats, e.g RGBA and size of each color
		std::vector<VkPresentModeKHR> presentationModes;	//How images should be presented to screen
	};

	struct SwapChainImage
	{
		VkImage image;
		VkImageView imageView; 
	};

	std::vector<char> readFile(const std::string& fileName);

	uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);

	void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void endAndSubmitCommitBuffer(VkDevice device, VkCommandPool commandPool,
		VkQueue queue, VkCommandBuffer commandBuffer);

	void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height);

	void transitionImageLayout(VkDevice device, VkQueue queue,
		VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
}