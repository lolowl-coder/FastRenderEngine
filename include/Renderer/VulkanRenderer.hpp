#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer/VulkanCommandBuffer.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanSwapchain.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Utilities.hpp"
#include "MeshModel.hpp"

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
	struct Shader;

	class VulkanRenderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer();
		
		int create(GLFWwindow* newWindow);
		void destroy();

		int createMeshModel(std::string modelFile,
			const std::vector<aiTextureType>& texturesLoadTypes);
		MeshModel* getMeshModel(int modelId);

		void draw(const Camera& camera, const glm::vec3& lightPosition);

		void setFramebufferResized(bool resized);

	protected:
		virtual void createGraphicsPipelines();

		virtual void cleanupGraphicsPipelines(VkDevice logicalDevice);

	protected:
		MainDevice mainDevice;
		int32_t mSubPassesCount = 0;

		VulkanRenderPass mRenderPass;

		std::vector<VulkanCommandBuffer> mCommandBuffers;
		
		VkPushConstantRange mModelMatrixPCR;
		VkPushConstantRange mLightingPCR;
		VkPushConstantRange mNearFarPCR;

		// - Descriptors
		VulkanDescriptorSetLayout mUniformDescriptorSetLayout;
		VulkanDescriptorSetLayout mInputDescriptorSetLayout;

		VulkanDescriptorPool mUniformDescriptorPool;
		VulkanDescriptorPool mInputDescriptorPool;

		std::vector<VulkanDescriptorSet> mUniformDescriptorSets;
		std::vector<VulkanDescriptorSet> mInputDescriptorSets;

		VulkanTextureManager mTextureManager;

		glm::vec3 mModelMn = glm::vec3(std::numeric_limits<float>::max());
    	glm::vec3 mModelMx = glm::vec3(std::numeric_limits<float>::min());

		// - Assets
		std::vector<MeshModel> mMeshModels;
		std::vector<Material> mMaterials;
		std::map<std::string, uint32_t> mTextureFileNameToIdMap;

		// - Dynamic data update functions
		void setViewport(uint32_t imageIndex);
		void setScissor(uint32_t imageIndex);

		// - Render
		void bindPipeline(uint32_t imageIndex, VkPipeline pipeline);
		virtual void onRenderModel(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
			const MeshModel& model, const Camera& camera, const glm::vec3& lightPosition);
		virtual void onRenderMesh(uint32_t imageIndex, VkCommandBuffer commandBuffer,
			VkPipelineLayout pipelineLayout, const MeshModel& model, const Mesh& mesh,
			const Camera& camera, const glm::vec3& lightPosition);
		void renderScene(uint32_t imageIndex, VkPipelineLayout pipelineLayout, const Camera& camera,
			const glm::vec3& lightPosition);
		void renderTexturedRect(uint32_t imageIndex, VkPipelineLayout pipelineLayout);
		virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex, const Camera& camera,
			const glm::vec3& lightPosition);

	private:
		const Shader& getShader(const Material& material) const;
		void buildMaterialToShaderMap();
		void loadShader(uint32_t shaderIndex);
		void loadUsedShaders();
		void loadTextures();

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

		//List of possible shaders
		std::vector<std::string> mRegisteredShaders;
		//Loaded shaders
		std::vector<Shader> mShaders;
		int mSceneShaderIndex = -1;
		std::string mFogShaderName;
		int mFogShaderId = -1;
		int mFogShaderIndex = -1;
		//To find shader by material
		std::map<uint32_t, uint32_t> mMaterialToShaderMap;

		VulkanPipeline mScenePipeline;
		VulkanPipeline mFogPipeline;

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
		void createCommandPool();
		void createCommandBuffers();
		void createUniformBuffers();
		void allocateUniformDescriptorSets();
		void allocateInputDescriptorSets();

		void updateUniformBuffers(uint32_t imageIndex, const Camera& camera);

		// - Record functions
		void recordCommands(uint32_t imageIndex, const Camera& camera,
			const glm::vec3& lightPosition);
			
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