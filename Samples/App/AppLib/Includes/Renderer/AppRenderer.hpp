#pragma once

#include "Renderer/VulkanAccelerationStructure.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/Denoiser.hpp"
#include "CudaBufferManager.hpp"
#include "ThreadPool.hpp"
#include "Pointers.hpp"

#include <string>

namespace app
{
	class AppRenderer : public fre::VulkanRenderer
	{
	public:
		AppRenderer(fre::ThreadPool& threadPool);

		virtual void initUI() override;
		virtual int createDynamicGPUResources() override;
		virtual int createMeshGPUResources() override;
		virtual void destroy() override;

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

	private:
        StorageImage mColorStorage;
        StorageImage mAlbedoStorage;
        StorageImage mNormalStorage;
		fre::MeshPtr mResultMesh;

        uint32_t mRTShaderId = MAX(uint32_t);

		fre::MeshModelPtr mMeshModel;
		fre::VulkanBuffer mVertexBuffer;
		fre::VulkanBuffer mIndexBuffer;
		fre::VulkanBuffer mTransformMatrixBuffer;
		fre::AccelerationStructure mBLAS;
		fre::AccelerationStructure mTLAS;
		fre::VulkanBuffer mSceneGPU;
		fre::VulkanDescriptorPtr mTLASDescriptor;
		fre::VulkanDescriptorPtr mRTMeshesGPUDescriptor;
		fre::VulkanDescriptorPtr mRTMaterialsGPUDescriptor;
		fre::VulkanDescriptorPtr mDynamicDataDescriptor;
		fre::VulkanDescriptorPtr mRTTexturesDescriptor;
		fre::VulkanDescriptorPtr mEmissiveTrianglesDescriptor;

		std::vector<fre::MeshPtr> mRTMeshes;
		fre::VulkanBuffer mRTMeshesGPUBuffer;
		uint32_t mRTMaterialsGPUBufferIndex = MAX(uint32_t);
		fre::VulkanBuffer mRTMaterialsGPUBuffer;
        uint32_t mDynamicDataBufferIndex = MAX(uint32_t);
        fre::VulkanBuffer mRTMeshesBuffer;
        fre::VulkanBuffer mRTMaterialsBuffer;
		fre::VulkanBuffer mDynamicDataBuffer;
		fre::VulkanBuffer mEmissiveTrianglesBuffer;
        std::vector<VkImageView> mTextureViews;
        std::vector<VkSampler> mTextureSamplers;
		uint32_t mShadowMissShaderId = MAX(uint32_t);

		//Interop

		//Cuda-Vulkan synchronization primitives
		cudaExternalSemaphore_t mCudaWaitSemaphore = nullptr;
		cudaExternalSemaphore_t mCudaSignalSemaphore = nullptr;

		cudaExternalMemory_t mCUDAExternalColorMem = nullptr;
		cudaExternalMemory_t mCUDAExternalAlbedoMem = nullptr;
		cudaExternalMemory_t mCUDAExternalNormalMem = nullptr;
        fre::CudaBuffer<float4> mCUDAExternalColorBuffer;
        fre::CudaBuffer<float4> mCUDAExternalAlbedoBuffer;
        fre::CudaBuffer<float4> mCUDAExternalNormalBuffer;

        fre::DynamicData mDynamicData;
		uint32_t mColorTextureId = MAX(uint32_t);
		uint32_t mAlbedoTextureId = MAX(uint32_t);
		uint32_t mNormalTextureId = MAX(uint32_t);
		fre::Denoiser mDenoiser;
		fre::CudaBufferManager mCudaBufferManager;
		VkSemaphore mExternalVulkanWaitSemaphore = VK_NULL_HANDLE;
		VkSemaphore mExternalVulkanSignalSemaphore = VK_NULL_HANDLE;
		bool mDenoiserInitialized = false;
		bool mIsDenoiserEnabled = true;
		uint32_t mLampMaterialIndex = MAX(uint32_t);
        std::vector<fre::Material> mDefaultMaterials;
	};
}