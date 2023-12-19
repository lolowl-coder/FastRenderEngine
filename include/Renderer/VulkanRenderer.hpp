#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>

#include "stb_image.h"
#include "Utilities.hpp"
#include "MeshModel.hpp"

class VulkanRenderer
{
public:
	VulkanRenderer();
	int init(GLFWwindow* newWindow);

	int createMeshModel(std::string modelFile);
	void updateModel(int mdoelId, glm::mat4 newModel);

	void draw();
	void cleanup();

	~VulkanRenderer();

private:
	GLFWwindow* window;

	int currentFrame = 0;

	//Scene settings
	struct UboViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	//Vulkan components
	VkInstance instance;

	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<SwapChainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFrameBuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkImage> colourBufferImage;
	std::vector<VkDeviceMemory> colourBufferImageMemory;
	std::vector<VkImageView> colourBufferImageView;

	std::vector<VkImage> depthBufferImage;
	std::vector<VkDeviceMemory> depthBufferImageMemory;
	std::vector<VkImageView> depthBufferImageView;

	VkSampler textureSampler;

	// - Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSetLayout inputSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	VkDescriptorPool inputDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;
	std::vector<VkDescriptorSet> inputDescriptorSets;

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	std::vector<VkBuffer> modelDUniformBuffer;
	std::vector<VkDeviceMemory> modelDUniformBufferMemory;


	/*VkDeviceSize minUniformBufferOffset;
	size_t modelUniformAlignment;
	Model* modetTransferSpace;*/

	// - Assets
	std::vector<MeshModel> modelList;

	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// - Pipeline
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkPipeline secondPipeline;
	VkPipelineLayout secondPipelineLayout;


	VkRenderPass renderPass;

	// - Pools -
	VkCommandPool graphicsCommandPool;

	// - Utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// - Synchronization
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	//Vulkan functions
	// -create functions
	void createInstance();

	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createColourBufferImage();
	void createDepthBufferImage();
	void createFrameBuffers();
	void createCommandPool();
	void createCommandBuffers();

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createInputDescriptorSets();

	void updateUniformBuffers(uint32_t imageIndex);

	// - Record functions
	void recordCommands(uint32_t currentImage);
		
	// - Get functions
	void getPhysicalDevice();

	// - Allocate function
	void allocateDynamicBufferTransferSpace();

	// -support functions
	// --Checker functions
	bool checkInstanceExtentionsSupport(std::vector<const char*>* checkExtentions);
	bool checkValidationLayerSupport();
	bool checkDeviceExtentionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// --Getter functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	// -- Choose functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// -- Create Functions
	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createSynchronisation();
	void createTextureSampler();

	int createTextureImage(std::string fileName);
	int createTexture(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	// -- Loader functions --
	stbi_uc* loatTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
};

