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
#include "Renderer/VulkanResourceCache.hpp"
#include "Renderer/VulkanCommandBuffer.hpp"
#include "Renderer/VulkanFrameBuffer.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderPass.hpp"
#include "Renderer/VulkanSamplerKeyHasher.hpp"
#include "Renderer/VulkanSwapchain.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Pointers.hpp"

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
	struct AccelerationStructure;
	class FeatureStorageBase;
	struct Camera;
	struct Light;
    struct ShaderInputParser;
	class ThreadPool;
	struct VulkanDescriptor;
	struct VulkanPipeline;

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

		//Descriptor sets
		uint32_t createDescriptorPool(const VulkanDescriptorPoolKey& key);
        VulkanDescriptorPoolPtr& getDescriptorPool(const uint32_t index);

		uint32_t createDescriptorSetLayout(const VulkanDescriptorSetLayoutInfo& key);
        VulkanDescriptorSetLayoutPtr& getDescriptorSetLayout(const uint32_t index);

		uint32_t createDescriptorSet(const VulkanDescriptorSetKey& key);
        VulkanDescriptorSetPtr& getDescriptorSet(const uint32_t index);

		void bindDescriptorSets(const std::vector<uint32_t>& setIds, VkPipelineLayout pipelineLayout, VkPipelineBindPoint pipelineBindPoint);

		//Descriptors
		uint32_t createSampler(const VulkanSamplerKey& key);
		VkSampler getSampler(const uint32_t index);

		uint32_t createTextureInfo(
			const VkSamplerAddressMode addressMode,
			const VkImageTiling tiling,
			const VkImageUsageFlags usageFlags,
			const VkMemoryPropertyFlags memoryFlags,
			const VkImageLayout layout,
			const bool isExternal,
			Image& image);
		VulkanTextureInfoPtr getTextureInfo(const uint32_t id);

		uint32_t createTexture(const VulkanTextureInfoPtr& info);
		VulkanTexturePtr getTexture(const uint32_t id);
		void updateTextureImage(const VulkanTextureInfoPtr& info);

		VulkanBuffer createStagingBuffer(const void* data, size_t size);
		const VulkanBuffer& createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, void* data, size_t dataSize);
		const VulkanBuffer& createExternalBuffer(VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryFlags,
			VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size);
		void copyBuffer(VkBuffer src, VkBuffer dst, size_t dataSize, VkPipelineBindPoint pipelineBindPoint) const;

		AccelerationStructure& createBLAS(VulkanBuffer& vbo, const uint32_t verticesCount, VulkanBuffer& ibo, const uint32_t indicesCount, VulkanBuffer& transform);
		AccelerationStructure& createTLAS(const uint64_t refBlasAddress, const VkTransformMatrixKHR& transform);
		AccelerationStructure& buildAccelerationStructure(VkAccelerationStructureGeometryKHR& acceleration_structure_geometry, const VkAccelerationStructureTypeKHR asType, const uint32_t primitiveCount);
		void destroyAccelerationStructure(AccelerationStructure& accelerationStructure);

		void addMaterial(Material& material);

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
		void pushConstants(VkPushConstantRange pushConstants, const void* data, VkPipelineLayout pipelineLayout, VkPipelineBindPoint pipelineBindPoint);
		void createBarrier(VkBuffer buffer, VkPipelineBindPoint pipelineBindPoint);
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

		VulkanFrameBuffer& getFramBuffer(){ return mFrameBuffers[mImageIndex]; }
		void transitionDepthLayout(VkImageLayout from, VkImageLayout to, VkPipelineBindPoint pipelineBindPoint);

		VulkanTextureManager& getTextureManager() { return mTextureManager; }

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
		BoundingBox2D getViewport() const;
		virtual void createPipelines();
		virtual void cleanupPipelines(VkDevice logicalDevice);
		virtual void createFullscreenTriangle();
		//Returns shader metadata associated with shader by its file name
		virtual ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName);

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
		void bindVertexBuffers(const VkBuffer* buffers, uint32_t count, VkDeviceSize* offsets, VkPipelineBindPoint pipelineBindPoint);
		void bindIndexBuffer(const VkBuffer buffer, VkPipelineBindPoint pipelineBindPoint);
		virtual void recordMeshCommands(
			const MeshModel::Ptr& model, const Mesh::Ptr& mesh, const Camera& camera,
			const Light& light, VkPipelineBindPoint pipelineBindPoint, uint32_t subPass, uint32_t instanceId);
		void recordSceneCommands(const Camera& camera, const Light& light, VkPipelineBindPoint pipelineBindPoint, uint32_t subPass);

		void renderFullscreenTriangle(VkPipelineLayout pipelineLayout);
		virtual void renderSubPass(uint32_t subPassIndex, const Camera& camera,
			const Light& light);

		virtual bool isRayTracingSupported() { return false; }

		void loadShaderStage(
			ShaderInputParser& parser,
			VulkanShader& shader,
			const std::string& shaderFileName,
			VkShaderStageFlagBits stage,
			std::vector<VulkanDescriptorSetLayoutInfo>& layouts);
		void loadShader(const std::string& shadeFilerName, std::unordered_map<VkDescriptorType, uint32_t>& descriptorTypes);
		void loadUsedShaders();
		//Load images in different thread
		void loadImages();
		//Creates GPU resources of loaded meshes
		virtual void loadMeshes();
		//Vulkan functions
		// -create functions
		void createInstance();

		void createLogicalDevice();
		virtual void createSwapChain();
		void createSwapChainFrameBuffers();
		void createSurface();
		void createUIDescriptorPool();
		void createCommandPools();
		void createCommandBuffers();
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

		virtual void cleanupUIDescriptorPool();
		virtual void cleanupSwapchainImagesSemaphores();
		virtual void cleanupRenderFinishedSemaphores();
		virtual void cleanupComputeFinishedSemaphores();
		virtual void cleanupDrawFences();
		virtual void cleanupComputeFences();
		virtual void cleanupTransferSynchronisation();
		virtual void cleanupSemaphores();
		virtual void cleanupUI();
		virtual void cleanupRayTracing();
        virtual void cleanupSwapChain();
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
		VulkanDescriptorPoolPtr mUIDescriptorPool;

		uint32_t mSharedDescriptorPoolId = MAX(uint32_t);

		VulkanResourceCache<VulkanSamplerKey, VkSampler> mSamplerCache;
		VulkanResourceCache<VulkanDescriptorPoolKey, VulkanDescriptorPoolPtr> mDescriptorPoolCache;
		VulkanResourceCache<VulkanDescriptorSetLayoutInfo, VulkanDescriptorSetLayoutPtr> mDescriptorSetLayoutCache;
		VulkanResourceCache<VulkanDescriptorSetKey, VulkanDescriptorSetPtr> mDescriptorSetCache;

		std::vector<VulkanDescriptorPtr> mColorAttacmentDescriptors;
		std::vector<VulkanDescriptorPtr> mDepthAttacmentDescriptors;

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

		uint32_t mImageIndex = std::numeric_limits<uint32_t>::max();
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;

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

		std::vector<VulkanQueueFamily> mQueueFamilies;
		int8_t mGraphicsQueueFamilyId = -1;
		int8_t mPresentationQueueFamilyId = -1;
		int8_t mTransferQueueFamilyId = -1;
		int8_t mComputeQueueFamilyId = -1;
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

		//std::vector<VkBuffer> mUniformBuffers;
		//std::vector<VkDeviceMemory> mVPUniformBufferMemory;

		/*std::vector<VkBuffer> modelDUniformBuffer;
		std::vector<VkDeviceMemory> modelDUniformBufferMemory;

		VkDeviceSize minUniformBufferOffset;
		size_t modelUniformAlignment;
		ModelMatrix* modetTransferSpace;*/

		VulkanSwapChain mSwapChain;
		std::vector<VulkanFrameBuffer> mFrameBuffers;

		// - Pools -
		VkCommandPool mTransferCommandPool = VK_NULL_HANDLE;
		VkCommandPool mComputeCommandPool = VK_NULL_HANDLE;

		// - Ray tracing
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR  mRayTracingPipelineProperties{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR mAccelerationStructureFeatures{};
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

        MeshPtr mFullscreenTriangleMesh;
	};
}