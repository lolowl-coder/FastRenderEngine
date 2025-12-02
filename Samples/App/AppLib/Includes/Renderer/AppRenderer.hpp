#pragma once

#include "Renderer/VulkanAccelerationStructure.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/Denoiser.hpp"
#include "CudaBufferManager.hpp"
#include "ThreadPool.hpp"
#include "Pointers.hpp"

#include <string>
#include<future>

namespace app
{
	class AppRenderer : public fre::VulkanRenderer
	{
	public:
		AppRenderer(fre::ThreadPool& threadPool);

		virtual void initUI(fre::Camera& camera) override;
		virtual int createDynamicGPUResources() override;
		virtual int createMeshGPUResources() override;
		virtual void destroy() override;
		virtual void onImageLoaded(const uint32_t imageIndex, const int loadQueueSize) override;

	protected:
		virtual void requestExtensions() override;
		virtual void requestDeviceFeatures() override;
		virtual bool isRayTracingSupported() { return true; }
        virtual void cleanupSwapChain() override;
        virtual void createSwapChain() override;
		virtual void update(const fre::Camera& camera, const fre::Light& light) override;
		virtual void onRaytracingCommandsSubmitted() override;

		virtual std::vector<const fre::VulkanShader*> getRTShaders(const uint32_t shaderId) override;
		virtual fre::ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) override;

	private:
		struct StorageImage
		{
			fre::VulkanTexturePtr texture;
            fre::VulkanDescriptorPtr descriptor;
		};
		StorageImage createStorageImage(bool external, VkFormat format, VkImageTiling tiling, const std::string& name);
		void loadMeshModel();
		void createResultMesh();
		void createAS();
		uint32_t createRTTexture(uint32_t textureId);
		void createSceneGPU();
		void initInterop();
		void createScene();
		void updateMaterials();
		void loadEnvTexture();
		void updateTextureDescriptors();

	private:
		//G-buffer storage images
        std::vector<StorageImage> mColorStorage;
        std::vector<StorageImage> mAlbedoStorage;
        std::vector<StorageImage> mNormalStorage;
        std::vector<StorageImage> mFlowStorage;
		//Final result mesh
		fre::MeshPtr mResultMesh;
		//Ray tracing shader id
        uint32_t mRTShaderId = MAX(uint32_t);
		//Loaded mesh model
		fre::MeshModelPtr mMeshModel;
		fre::VulkanBuffer mVertexBuffer;
		fre::VulkanBuffer mIndexBuffer;
		fre::VulkanBuffer mTransformMatrixBuffer;

		//Ray tracing
		
		//Acceleration structures
		fre::AccelerationStructure mBLAS;
		fre::AccelerationStructure mTLAS;
		//Descriptors for ray tracing
		fre::VulkanDescriptorPtr mTLASDescriptor;
		fre::VulkanDescriptorPtr mRTMeshesGPUDescriptor;
		fre::VulkanDescriptorPtr mRTMaterialsGPUDescriptor;
		fre::VulkanDescriptorPtr mDynamicDataDescriptor;
		fre::VulkanDescriptorPtr mRTTexturesDescriptor;
		fre::VulkanDescriptorPtr mEmissiveTrianglesDescriptor;

		//CPU-side meshes for ray tracing
		std::vector<fre::MeshPtr> mRTMeshes;

		//GPU-side buffers

		//RT meshes buffer
		uint32_t mRTMeshesGPUBufferIndex = MAX(uint32_t);
		fre::VulkanBuffer mRTMeshesGPUBuffer;
		//RT materials buffer
		uint32_t mRTMaterialsGPUBufferIndex = MAX(uint32_t);
		fre::VulkanBuffer mRTMaterialsGPUBuffer;
		//Uniform buffer with RT options
        uint32_t mDynamicDataBufferIndex = MAX(uint32_t);
		fre::VulkanBuffer mDynamicDataBuffer;
		//Emissive triangles buffer (area lights)
		fre::VulkanBuffer mEmissiveTrianglesBuffer;
		//Texture views and samplers we pass to ray tracing shaders
        std::vector<VkImageView> mTextureViews;
        std::vector<VkSampler> mTextureSamplers;
		//Shadow miss shader id to build SBT
		uint32_t mShadowMissShaderId = MAX(uint32_t);

		//Interop btw CUDA and Vulkan

		//Cuda-Vulkan synchronization primitives
		std::vector<cudaExternalSemaphore_t> mCudaWaitSemaphores;
		std::vector<cudaExternalSemaphore_t> mCudaSignalSemaphores;
		VkSemaphore mExternalVulkanWaitSemaphore = VK_NULL_HANDLE;
		VkSemaphore mExternalVulkanSignalSemaphore = VK_NULL_HANDLE;
		//Buffers to feed to denoiser
        std::vector<fre::CudaBuffer<float4>> mCUDAExternalColorBuffer;
        std::vector<fre::CudaBuffer<float4>> mCUDAExternalAlbedoBuffer;
        std::vector<fre::CudaBuffer<float4>> mCUDAExternalNormalBuffer;
        std::vector<fre::CudaBuffer<float4>> mCUDAExternalFlowBuffer;
		//Cuda buffer manager to simplify allocations/cleanup
		fre::CudaBufferManager mCudaBufferManager;

		//Denoiser

		//OptiX denoiser instance
		fre::Denoiser mDenoiser;
		bool mDenoiserInitialized = false;
		//Real time denoiser enabled flag
		bool mIsDenoiserEnabled = true;

		//Uniform buffer with settings we edit via UI
        fre::DynamicData mDynamicData;
		//Temporal mode.
		//TODO: not implemented fully yet. No improvements if turned on. I suspect smth is missed out.
		bool mTemporalMode = false;
		//Index of lamp material (for pool2 scene)
		uint32_t mLampMaterialIndex = MAX(uint32_t);
		//Materials backup list to restore from if we edited them via options
        std::vector<fre::Material> mDefaultMaterials;
		//Progressive ray tracing frame counter
		int mAccumulatedFrames = 0;
		//Environment texture id
		int mEnvTexIndex = 0;
		bool mAllTexturesCreated = false;
		//Asynchronous texture loading
		std::future<fre::VulkanTextureInfoPtr> mTextureFuture;
		//Progress bar value
		float mProgress = -1.0f;
		//Loaded images counter
		int mLoadedImageCount = 0;
		//Last loaded image file name
		std::string mProgressTextOverlay;
		std::mutex mMutex;
	};
}