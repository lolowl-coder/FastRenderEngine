#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>

#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanCommandBuffer.hpp"
#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanSwapchain.hpp"
#include "Utilities.hpp"
#include "stb_image.h"
#include "MeshModel.hpp"

namespace fre
{
	struct VulkanPipeline;

	class VulkanRenderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer();
		
		int create(GLFWwindow* newWindow);
		void destroy();

		int createMeshModel(std::string modelFile);
		void updateModel(int mdoelId, glm::mat4 newModelMatrix);

		void draw();

		void setFramebufferResized(bool resized);


	protected:
		virtual void createGraphicsPipelines();

		virtual void cleanupGraphicsPipelines(VkDevice logicalDevice);

	protected:
		MainDevice mainDevice;
		int32_t mSubPassesCount = 0;

		VulkanRenderPass mRenderPass;

		// - Descriptors
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSetLayout samplerSetLayout;
		VkDescriptorSetLayout inputSetLayout;

		// - inputs
		VkPushConstantRange pushConstantRange;

		// - Dynamic data update functions
		void setViewport(uint32_t imageIndex);
		void setScissor(uint32_t imageIndex);

		// - Render
		void bindPipeline(uint32_t imageIndex, const VulkanPipeline& pipeline);
		void renderScene(uint32_t imageIndex, const VulkanPipeline& pipeline);
		void renderTexturedRect(uint32_t imageIndex, VulkanPipeline& pipeline);
		virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex);

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

		VkQueue graphicsQueue;
		VkQueue presentationQueue;
		VkSurfaceKHR surface;
		std::vector<VulkanCommandBuffer> mCommandBuffers;

		VkSampler textureSampler;

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
		ModelMatrix* modetTransferSpace;*/

		// - Assets
		std::vector<MeshModel> modelList;

		std::vector<VkImage> textureImages;
		std::vector<VkDeviceMemory> textureImageMemory;
		std::vector<VkImageView> textureImageViews;

		VulkanSwapChain mSwapChain;
		std::vector<VulkanFrameBuffer> mFrameBuffers;

		// - Pools -
		VkCommandPool graphicsCommandPool;

		// - Synchronization
		std::vector<VkSemaphore> imageAvailable;
		std::vector<VkSemaphore> renderFinished;
		std::vector<VkFence> drawFences;

		bool framebufferResized = false;

		//Vulkan functions
		// -create functions
		void createInstance();

		void createLogicalDevice();
		void createSwapChainFrameBuffers();
		void createSurface();
		void createDescriptorSetLayout();
		void createInputDescriptorSetLayout();
		void createPushConstantRange();
		void createCommandPool();
		void createCommandBuffers();

		void createUniformBuffers();
		void createDescriptorPool();
		void createInputDescriptorPool();
		void createDescriptorSets();
		void createInputDescriptorSets();

		void updateUniformBuffers(uint32_t imageIndex);

		// - Record functions
		void recordCommands(uint32_t imageIndex);
			
		// - Get functions
		void getPhysicalDevice();

		// - Allocate function
		void allocateDynamicBufferTransferSpace();

		// - Cleanup methods
		void cleanupSwapChainFrameBuffers();
		void cleanupInputDescriptorPool();
		void cleanupSwapchainImagesSemaphores();
		void cleanupRenderFinishedSemaphors();
		void cleanupDrawFences();

		// - Recreate methods
		void recreateSwapChain();

		// - Dynamic data update functions
		void updateProjectionMatrix();

		// -support functions
		// --Checker functions
		bool checkInstanceExtentionsSupport(std::vector<const char*>* checkExtentions);
		bool checkValidationLayerSupport();
		bool checkDeviceExtentionSupport(VkPhysicalDevice device);
		bool checkDeviceSuitable(VkPhysicalDevice device);
		
		void createSwapchainImagesSemaphores();
		void createRenderFinishedSemaphores();
		void createDrawFences();
		void createSynchronisation();
		void createTextureSampler();

		int createTextureImage(std::string fileName);
		int createTexture(std::string fileName);
		int createTextureDescriptor(VkImageView textureImage);

		// -- Loader functions --
		stbi_uc* loatTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
	};
}