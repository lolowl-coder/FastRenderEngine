#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanCommandBuffer.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanSwapchain.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "MeshModel.hpp"
#include "ThreadPool.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "Utilities.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

namespace fre
{
	struct VulkanPipeline;
	struct Camera;
	struct Light;

	class VulkanRenderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer();
		
		int create(GLFWwindow* newWindow);
		void destroy();

		void addMaterial(Material& material);
		int createMeshModel(std::string modelFile,
			const std::vector<aiTextureType>& texturesLoadTypes);
		void addMeshModel(const MeshModel& meshModel);
		MeshModel* getMeshModel(int modelId);

		void draw(const Camera& camera, const Light& light);
		void drawUI(uint32_t imageIndex);

		void setFramebufferResized(bool resized);

		void setClearColor(const glm::vec4& clearColor);

	protected:
		virtual void createGraphicsPipelines();

		virtual void cleanupGraphicsPipelines(VkDevice logicalDevice);

	protected:
		MainDevice mainDevice;
		int32_t mSubPassesCount = 0;

		VulkanRenderPass mRenderPass;

		std::vector<VulkanCommandBuffer> mGraphicsCommandBuffers;
		std::vector<VulkanCommandBuffer> mTransferCommandBuffers;
		
		VkPushConstantRange mModelMatrixPCR;
		VkPushConstantRange mLightingPCR;
		VkPushConstantRange mNearFarPCR;

		// - Descriptors
		VulkanDescriptorSetLayout mUniformDescriptorSetLayout;
		VulkanDescriptorSetLayout mInputDescriptorSetLayout;

		VulkanDescriptorPool mUniformDescriptorPool;
		VulkanDescriptorPool mInputDescriptorPool;
		VulkanDescriptorPool mUIDescriptorPool;

		std::vector<VulkanDescriptorSet> mUniformDescriptorSets;
		std::vector<VulkanDescriptorSet> mInputDescriptorSets;

		VulkanBufferManager mBufferManager;
		VulkanTextureManager mTextureManager;

		glm::vec3 mModelMn = glm::vec3(std::numeric_limits<float>::max());
    	glm::vec3 mModelMx = glm::vec3(std::numeric_limits<float>::min());

		// - Assets
		std::vector<MeshModel> mMeshModels;
		std::vector<Material> mMaterials;
		std::map<std::string, uint32_t> mTextureFileNameToIdMap;
		std::map<uint32_t, uint32_t> mMeshToVertexBufferMap;
		std::map<uint32_t, uint32_t> mMeshToIndexBufferMap;

		// - Dynamic data update functions
		void setViewport(uint32_t imageIndex);
		void setScissor(uint32_t imageIndex);

		// - Render
		void bindPipeline(uint32_t imageIndex, VkPipeline pipeline);
		virtual void onRenderMesh(uint32_t imageIndex, VkCommandBuffer commandBuffer,
			const MeshModel& model, const Mesh& mesh, const Camera& camera,
			const Light& light);
		void renderScene(uint32_t imageIndex, const Camera& camera,
			const Light& light);
		void renderTexturedRect(uint32_t imageIndex, VkPipelineLayout pipelineLayout);
		virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex, const Camera& camera,
			const Light& light);

	private:
		ShaderMetaData getShaderMetaData(const std::string& shaderFileName) const;
		void loadShader(const std::string& shadeFilerName);
		void loadUsedShaders();
		void loadImages();
		void loadMeshes();

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
		VkInstance instance = VK_NULL_HANDLE;

		std::vector<VulkanQueueFamily> mQueueFamilies;
		int8_t mGraphicsQueueFamilyId = -1;
		int8_t mPresentationQueueFamilyId = -1;
		int8_t mTransferQueueFamilyId = -1;
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentationQueue = VK_NULL_HANDLE;
		VkQueue mTransferQueue = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		//Loaded shaders
		std::vector<Shader> mShaders;
		//Loaded resources
		std::vector<std::string> mTextureFileNames;
		std::vector<std::string> mShaderFileNames;
		std::vector<ShaderMetaData> mShaderMetaData;

		std::vector<VulkanPipeline> mPipelines;

		int mFogShaderId = -1;

		std::vector<VkBuffer> vpUniformBuffer;
		std::vector<VkDeviceMemory> vpUniformBufferMemory;

		/*std::vector<VkBuffer> modelDUniformBuffer;
		std::vector<VkDeviceMemory> modelDUniformBufferMemory;

		VkDeviceSize minUniformBufferOffset;
		size_t modelUniformAlignment;
		ModelMatrix* modetTransferSpace;*/

		VulkanSwapChain mSwapChain;
		std::vector<VulkanFrameBuffer> mFrameBuffers;

		// - Pools -
		VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;
		VkCommandPool mTransferCommandPool = VK_NULL_HANDLE;

		// - Synchronization
		std::vector<VkSemaphore> imageAvailable;
		std::vector<VkSemaphore> renderFinished;
		std::vector<VkFence> drawFences;
		VkSemaphore mTransferCompleteSemaphore = VK_NULL_HANDLE;
		VkFence mTransferFence = VK_NULL_HANDLE;

		bool framebufferResized = false;

		glm::vec4 mClearColor = glm::vec4(0.0f);

		ThreadPool mThreadPool;
		std::mutex mTextureMutex;
		std::mutex mBufferMutex;

		Statistics mStatistics;

		//Vulkan functions
		// -create functions
		void createInstance();

		void createLogicalDevice();
		void createSwapChainFrameBuffers();
		void createSurface();
		void createUniformDescriptorPool();
		void createInputDescriptorPool();
		void createUIDescriptorPool();
		void createUniformDescriptorSetLayout();
		void createInputDescriptorSetLayout();
		void createCommandPools();
		void createCommandBuffers();
		void createUniformBuffers();
		void allocateUniformDescriptorSets();
		void allocateInputDescriptorSets();
		void createUI();

		void updateUniformBuffers(uint32_t imageIndex, const Camera& camera);

		// - Record functions
		void recordCommands(uint32_t imageIndex, const Camera& camera,
			const Light& light);
			
		// - Get functions
		void getPhysicalDevice();

		// - Allocate function
		void allocateDynamicBufferTransferSpace();

		// - Cleanup methods
		void cleanupSwapChainFrameBuffers();

		void cleanupUniformDescriptorPool();
		void cleanupInputDescriptorPool();
		void cleanupUIDescriptorPool();
		void cleanupUniformDescriptorSetLayout();
		void cleanupInputDescriptorSetLayout();
		void cleanupSwapchainImagesSemaphores();
		void cleanupRenderFinishedSemaphores();
		void cleanupDrawFences();
		void cleanupTransferSynchronisation();
		void cleanupUI();

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
		void createTransferSynchronisation();
	};
}