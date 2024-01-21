#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/Camera.hpp"
#include "Renderer/VulkanCommandBuffer.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanSwapchain.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Utilities.hpp"
#include "MeshModel.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>

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
		void updateModel(int modelId, glm::mat4 newModelMatrix);

		void draw();

		void setFramebufferResized(bool resized);

	protected:
		virtual void createGraphicsPipelines();

		virtual void cleanupGraphicsPipelines(VkDevice logicalDevice);

	protected:
		MainDevice mainDevice;
		int32_t mSubPassesCount = 0;

		VulkanRenderPass mRenderPass;

		std::vector<VulkanCommandBuffer> mCommandBuffers;

		// - Descriptors
		VulkanDescriptorSetLayout mUniformDescriptorSetLayout;
		VulkanDescriptorSetLayout mInputDescriptorSetLayout;

		VulkanDescriptorPool mUniformDescriptorPool;
		VulkanDescriptorPool mInputDescriptorPool;

		std::vector<VulkanDescriptorSet> mUniformDescriptorSets;
		std::vector<VulkanDescriptorSet> mInputDescriptorSets;

		VulkanTextureManager mTextureManager;

		// - Push constants
		VkPushConstantRange pushConstantRange;
		VkPushConstantRange pushConstantRangeNearFar;

		Camera mCamera;

		// - Dynamic data update functions
		void setViewport(uint32_t imageIndex);
		void setScissor(uint32_t imageIndex);

		// - Render
		void bindPipeline(uint32_t imageIndex, VkPipeline pipeline);
		virtual void onRenderModel(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
			const MeshModel& meshModel);
		void renderScene(uint32_t imageIndex, VkPipelineLayout pipelineLayout);
		void renderTexturedRect(uint32_t imageIndex, VkPipelineLayout pipelineLayout);
		virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex);
		virtual void createCamera();

	private:
		GLFWwindow* window;

		int currentFrame = 0;

		//Scene settings
		struct UboViewProjection
		{
			glm::mat4 projection;
			glm::mat4 view;
		};

		//Vulkan components
		VkInstance instance;

		VkQueue graphicsQueue;
		VkQueue presentationQueue;
		VkSurfaceKHR surface;

		std::vector<VkBuffer> vpUniformBuffer;
		std::vector<VkDeviceMemory> vpUniformBufferMemory;

		/*std::vector<VkBuffer> modelDUniformBuffer;
		std::vector<VkDeviceMemory> modelDUniformBufferMemory;

		VkDeviceSize minUniformBufferOffset;
		size_t modelUniformAlignment;
		ModelMatrix* modetTransferSpace;*/

		// - Assets
		std::vector<MeshModel> mMeshModels;
		std::vector<Material> mMaterials;

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
		void createUniformDescriptorPool();
		void createInputDescriptorPool();
		void createUniformDescriptorSetLayout();
		void createInputDescriptorSetLayout();
		void createPushConstantRange();
		void createCommandPool();
		void createCommandBuffers();
		void createUniformBuffers();
		void allocateUniformDescriptorSets();
		void allocateInputDescriptorSets();

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
	};
}