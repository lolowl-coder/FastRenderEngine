#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Light.hpp"
#include "MeshModel.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "Utilities.hpp"
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
	class ThreadPool;
	struct AccelerationStructure;
	class FeatureStorageBase;

	class VulkanRenderer
	{
	public:
		using UIRenderCallback = std::function<void()>;
		VulkanRenderer(ThreadPool& threadPool);
		virtual ~VulkanRenderer();
		
		virtual int createCoreGPUResources(GLFWwindow* newWindow);
		virtual int createDynamicGPUResources();
		virtual int createMeshGPUResources();
		virtual int createLoadableGPUResources();
		virtual int postCreate();
		virtual void destroy();
		virtual void destroyGPUResources();

		VulkanDescriptorPool::Ptr& addDescriptorPool(VkDescriptorPoolCreateFlags flags, uint32_t count,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		VulkanDescriptorSetLayout::Ptr& addDescriptorSetLayout(const std::vector<VkDescriptorType>& descriptorTypes,
			const std::vector<uint32_t>& stageFlags);
		VulkanDescriptorSet::Ptr& addDescriptorSet(
			const VkDescriptorPool& descriptorPool,
			const VkDescriptorSetLayout& descriptorSetLayout);

		void addMaterial(Material& material);
		int addTexture(const std::string& textureFileName, bool isExternal);
		int createTexture(Image& image);
		Image* createImage(const std::string& fileName, bool isExternal) { return &mTextureManager.createImage(fileName, isExternal); }
		Image* createImage(bool isExternal) { return &mTextureManager.createImage(isExternal); }
		Image* getImage(uint32_t id);

		AccelerationStructure& createAccelerationStructure();
		void destroyAccelerationStructure(AccelerationStructure& accelerationStructure);

		void updateTextureImage(uint32_t imageId, Image& image);
		int addShader(const std::string& shaderFileName);
		//Load model file
		MeshModel::Ptr& createMeshModel(std::string modelFile,
			const std::vector<aiTextureType>& texturesLoadTypes);
		//Add model to list
		MeshModel::Ptr& addMeshModel(const MeshModel::MeshList& meshList);
		MeshModel::Ptr getMeshModel(uint32_t modelId) const;
		MeshModel::Ptr getMeshModel(uint32_t modelId);

		//Update
		virtual void update(const Camera& camera, const Light& light);
		//Draw all models and UI
		virtual void draw(const Camera& camera, const Light& light);
		void preprocessUI();
		void drawUI();

		const MainDevice& getMainDevice(){ return mainDevice; }
		uint32_t getImageIndex(){ return mImageIndex; }
		uint32_t getCurrentFrameIndex(){ return mCurrentFrame; }
		//Push shader constants
		void pushConstants(VkPushConstantRange pushConstants, const void* data, VkPipelineLayout pipelineLayout, bool compute);
		//Bind descriptor sets
		void bindDescriptorSets(VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& sets, bool compute);
		VulkanBuffer createStagingBuffer(const void* data, size_t size);
		const VulkanBuffer& createBuffer(uint32_t usage, VkMemoryPropertyFlags memoryFlags, void* data, size_t dataSize);
		const VulkanBuffer& createExternalBuffer(int bufferUsage, VkMemoryPropertyFlags memoryFlags,
			VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size);
		void copyBuffer(VkBuffer src, VkBuffer dst, size_t dataSize, bool compute) const;
		void createBarrier(VkBuffer buffer, bool compute);
		const VulkanBuffer* getVertexBuffer(const uint32_t meshId) const;
		const VulkanBuffer* getIndexBuffer(const uint32_t meshId) const;
		//External shader meta data provider
		void setShaderMetaDataProvider(ShaderMetaDataProvider* provider);
		//Shader used if no shader was selected for material
		void setDefaultShaderFileName(const std::string& shaderFileName);
		
		//Sets "resized" flag
		void setFramebufferResized(bool resized);
		void setClearColor(const glm::vec4& clearColor);

		//Default shininess if material loaded from scene does not have this property
		void setDefaultShininess(const float shininess);

		VulkanDescriptorPool::Ptr& getDP(uint32_t id){ return mDescriptorPools[id]; }
		VulkanDescriptorSetLayout::Ptr& getDSL(uint32_t id){ return mDescriptorSetLayouts[id]; }
		VulkanDescriptorSet::Ptr& getDS(uint32_t id){ return mDescriptorSets[id]; }

		VulkanFrameBuffer& getFramBuffer(){ return mFrameBuffers[mImageIndex]; }

		VkDescriptorSetLayout getInputDSL() const { return mInputDescriptorSetLayout.mDescriptorSetLayout; }
		VkDescriptorSet getInputDS() const { return mInputDescriptorSets[mImageIndex].mDescriptorSet; }
		
		VkDescriptorSetLayout getDepthDSL() const { return mDepthDescriptorSetLayout.mDescriptorSetLayout; }
		VkDescriptorSet getDepthDS(uint32_t index) const { return mDepthDescriptorSets[index].mDescriptorSet; }
		void transitionDepthLayout(VkImageLayout from, VkImageLayout to, bool compute);

		//Returns uniforms descriptor set layout
		VkDescriptorSetLayout getUniformDSL() const { return mUniformDescriptorSetLayout.mDescriptorSetLayout; }
		VkDescriptorSet getUniformDS()
		{
			return mUniformDescriptorSets[mImageIndex].mDescriptorSet;
		}
		//Returns sampler descriptor set layout
		VkDescriptorSetLayout getSamplerDSL() const { return mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout; }
		VulkanTextureManager& getTextureManager() { return mTextureManager; }
		const VkDescriptorSet& getSamplerDS(uint32_t textureId, bool compute)
		{
			return mTextureManager.getDescriptorSet(mainDevice, mTransferQueueFamilyId,
				compute ? mComputeQueueFamilyId : mGraphicsQueueFamilyId,
				compute ? mComputeQueue : mGraphicsQueue,
				compute ? mGraphicsCommandPool : mComputeCommandPool, textureId).mDescriptorSet;
		}

		VkSemaphore getExternalWaitSemaphore() { return mExternalWaitSemaphore; }
		VkSemaphore getExternalSignalSemaphore() { return mExternalSignalSemaphore; }

		void readFromGPUMemory(VkDeviceMemory memory, void* dstBuffer, size_t size);

		//Returns common push constant range for model matrix
		VkPushConstantRange getModelMatrixPCR() const { return mModelMatrixPCR; }
		//Returns common push constant range for lighting
		VkPushConstantRange getLightingPCR() const { return mLightingPCR; }
		//Fills lighting push constants
		void fillLightingPushConstant(const Mesh::Ptr& mesh, const glm::mat4& modelMatrix,
			const Camera& camera, const Light& light, Lighting& lighting);

		Material& getMaterial(uint32_t id);

		void requestRedraw();
		void resetRedrawRequest();
		bool needRedraw();

		void addUIRenderCallback(const UIRenderCallback& callback) { mUIRenderCallbacks.push_back(callback); }
		
		// - Dynamic data update functions
		void setViewport(const BoundingBox2D& viewport);
		void setScissor(const BoundingBox2D& scissorRect);

		uint8_t* getDeviceUID() { return mDeviceUUID; }

		void* getMemHandle(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagBits handleType);
		void* getSemaphoreHandle(VkSemaphore semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType);

		void setHasExternalResources(bool hasExternalResources) { mHasExternalResources = hasExternalResources; }

	protected:
		virtual void createPipelines();
		virtual void cleanupPipelines(VkDevice logicalDevice);
		//Returns shader metadata associated with shader by its file name
		virtual ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName);

	protected:
		MainDevice mainDevice;

		void** mLastDeviceFeatures = nullptr;
		std::vector<std::unique_ptr<FeatureStorageBase>> mFeatureChain;
		std::vector<VkBool32*> mDeviceFeaturesEnabled;

		int32_t mSubPassesCount = 0;

		VulkanRenderPass mRenderPass;

		std::vector<VulkanCommandBuffer> mGraphicsCommandBuffers;
		std::vector<VulkanCommandBuffer> mTransferCommandBuffers;
		std::vector<VulkanCommandBuffer> mComputeCommandBuffers;
		
		// - Push constants
		VkPushConstantRange mModelMatrixPCR;
		VkPushConstantRange mLightingPCR;
		VkPushConstantRange mNearFarPCR;

		Lighting mLighting;

		// - Descriptors
		VulkanDescriptorSetLayout mUniformDescriptorSetLayout;
		VulkanDescriptorSetLayout mInputDescriptorSetLayout;
		VulkanDescriptorSetLayout mDepthDescriptorSetLayout;

		VulkanDescriptorPool mUniformDescriptorPool;
		VulkanDescriptorPool mInputDescriptorPool;
		VulkanDescriptorPool mDepthDescriptorPool;
		VulkanDescriptorPool mUIDescriptorPool;

		std::vector<VulkanDescriptorPool::Ptr> mDescriptorPools;
		std::vector<VulkanDescriptorSetLayout::Ptr> mDescriptorSetLayouts;
		std::vector<VulkanDescriptorSet::Ptr> mDescriptorSets;

		std::vector<VulkanDescriptorSet> mUniformDescriptorSets;
		std::vector<VulkanDescriptorSet> mInputDescriptorSets;
		std::vector<VulkanDescriptorSet> mDepthDescriptorSets;

		VulkanBufferManager mBufferManager;
		VulkanTextureManager mTextureManager;

		//Whole scene bounding box
		BoundingBox3D mSceneBoundingBox;

		// - Assets
		std::vector<MeshModel::Ptr> mMeshModels;
		std::vector<Material> mMaterials;
		//Default shininess if it can't be extracted during scene loading
		float mDefaultShininess = 16.0f;
		//Texture file name to Texture Id map
		std::map<std::string, uint32_t> mTextureFileNameToIdMap;
		//Mesh Id to vertex buffer map
		std::map<uint32_t, uint32_t> mMeshToVertexBufferMap;
		//Mesh Id to index buffer map
		std::map<uint32_t, uint32_t> mMeshToIndexBufferMap;

	protected:
		inline void setApiVersion(uint32_t requestedApiVersion)
		{
			mApiVersion = requestedApiVersion;
		}

		inline void addInstanceExtension(const char* extension)
		{
			mRequestedInstanceExtensions.push_back(extension);
		}

		inline void addDeviceExtension(const char* extension)
		{
			mRequestedDeviceExtensions.push_back(extension);
		}

		//Extensions
		virtual void requestExtensions();
		virtual void requestDeviceFeatures();

		// - Render
		void bindPipeline(const VulkanPipeline& pipeline);
		void bindVertexBuffers(const VkBuffer* buffers, uint32_t count, VkDeviceSize* offsets, bool compute);
		void bindIndexBuffer(const VkBuffer buffer, bool compute);
		virtual void recordMeshCommands(
			const MeshModel::Ptr& model, const Mesh::Ptr& mesh, const Camera& camera,
			const Light& light, bool compute, uint32_t subPass, uint32_t instanceId);
		void recordSceneCommands(const Camera& camera, const Light& light, bool compute, uint32_t subPass);

		void renderFullscreenTriangle(VkPipelineLayout pipelineLayout);
		virtual void renderSubPass(uint32_t subPassIndex, const Camera& camera,
			const Light& light);

	private:
		void loadShader(const std::string& shadeFilerName);
		void loadUsedShaders();
		//Load images in different thread
		void loadImages();
		//Creates GPU resources of loaded meshes
		virtual void loadMeshes();
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

		void updateUniformBuffers(const Camera& camera);

		// - Record functions
		void recordCommands(const Camera& camera,
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
		void cleanupInputDescriptorSetLayout();
		void cleanupUniformDescriptorSetLayout();
		void cleanupSwapchainImagesSemaphores();
		void cleanupRenderFinishedSemaphores();
		void cleanupComputeFinishedSemaphores();
		void cleanupDrawFences();
		void cleanupComputeFences();
		void cleanupTransferSynchronisation();
		void cleanupSemaphores();
		void cleanupUI();
		void cleanupRayTracing();

		// - Recreate methods
		void recreateSwapChain();

		// -support functions
		// --Checker functions
		bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions);
		bool checkValidationLayerSupport();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkDeviceFeaturesSupport();
		bool checkDeviceSuitable(VkPhysicalDevice device);
		
		void createSwapchainImagesSemaphores();
		void createRenderFinishedSemaphores();
		void createComputeFinishedSemaphores();
		VkSemaphore createExternalSemaphore(VkExternalSemaphoreHandleTypeFlagBits handleType);
		void createExternalSemaphores();
		void createDrawFences();
		void createComputeFences();
		void createSynchronisation();
		void createTransferSynchronisation();
		void initRayTracing();

	private:
		
		GLFWwindow* mWindow;

		int mCurrentFrame = 0;

		//Scene settings
		struct UboViewProjection
		{
			glm::mat4 mProjection;
			glm::mat4 mView;
		};

		//Vulkan components
		VkInstance mInstance;

		//Requested API version
		uint32_t mApiVersion = VK_API_VERSION_1_0;

		//Extensions
		std::vector<const char*> mRequestedDeviceExtensions;
		std::vector<const char*> mRequestedInstanceExtensions;
		VkPhysicalDeviceFeatures2 mDeviceFeatures = {};

		uint32_t mImageIndex = std::numeric_limits<uint32_t>::max();

		std::vector<VulkanQueueFamily> mQueueFamilies;
		int8_t mGraphicsQueueFamilyId = -1;
		int8_t mPresentationQueueFamilyId = -1;
		int8_t mTransferQueueFamilyId = -1;
		int8_t mComputeQueueFamilyId = -1;
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentationQueue = VK_NULL_HANDLE;
		VkQueue mTransferQueue = VK_NULL_HANDLE;
		VkQueue mComputeQueue = VK_NULL_HANDLE;
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;

		//Loaded shaders
		std::vector<Shader> mShaders;
		std::string mDefaultShaderFileName = "colored";
		//Loaded resources
		std::vector<std::string> mShaderFileNames;
		std::vector<ShaderMetaDatum> mShaderMetaDatum;
		ShaderMetaDataProvider* mShaderMetaDataProvider = nullptr;

		std::vector<VulkanPipeline> mPipelines;

		int mFogShaderId = -1;

		std::vector<VkBuffer> mVPUniformBuffer;
		std::vector<VkDeviceMemory> mVPUniformBufferMemory;

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
		VkCommandPool mComputeCommandPool = VK_NULL_HANDLE;

		// - Acceleration structures
		std::vector<AccelerationStructure> mAccelerationStructures;

		// - Synchronization
		std::vector<VkSemaphore> mImageAvailable;
		std::vector<VkSemaphore> mRenderFinished;
		std::vector<VkSemaphore> mComputeFinished;
		std::vector<VkFence> mDrawFences;
		std::vector<VkFence> mComputeFences;
		VkSemaphore mTransferCompleteSemaphore = VK_NULL_HANDLE;
		std::vector<VkSemaphore> mSemaphores;
		VkSemaphore mExternalWaitSemaphore = VK_NULL_HANDLE;
		VkSemaphore mExternalSignalSemaphore = VK_NULL_HANDLE;
		bool mHasExternalResources = false;
		VkFence mTransferFence = VK_NULL_HANDLE;

		bool mFramebufferResized = false;

		glm::vec4 mClearColor = glm::vec4(0.0f);

		ThreadPool& mThreadPool;

		Statistics mStatistics;

		uint32_t mFrameNumber = 0;
		bool mHasComputeTasks = false;
		
        int32_t mNeedRedraw = 5;

		std::vector<UIRenderCallback> mUIRenderCallbacks;

		uint8_t mDeviceUUID[VK_UUID_SIZE];
		
		bool mUIFrameStarted = false;
	};
}