#pragma once

#include "Renderer/VulkanAccelerationStructure.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "ThreadPool.hpp"
#include "Pointers.hpp"

#include <string>

namespace app
{
	class AppRenderer : public fre::VulkanRenderer
	{
	public:
		AppRenderer(fre::ThreadPool& threadPool)
			: VulkanRenderer(threadPool)
		{
		}

		virtual int createDynamicGPUResources() override;
		virtual int createLoadableGPUResources() override;
		virtual int createMeshGPUResources() override;

	protected:
		virtual void requestExtensions() override;
		virtual void requestDeviceFeatures() override;
		virtual bool isRayTracingSupported() { return true; }
        virtual void cleanupSwapChain() override;
        virtual void createSwapChain() override;
		virtual void update(const fre::Camera& camera, const fre::Light& light) override;

		virtual fre::ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) override;

	private:
		void createStorageImage();
		void loadMeshModel();
		void createResultMesh();
		void createAS();
		uint32_t createRTTexture(uint32_t textureId);
		void createSceneGPU();
		void createScene();

	private:
        fre::VulkanTexturePtr mStorageImage;
		fre::VulkanDescriptorPoolPtr mStorageImageDP;
		fre::VulkanDescriptorSetLayoutPtr mStorageImageDSL;
		fre::VulkanDescriptorSetPtr mStorageImageDS;
		fre::VulkanDescriptorPoolPtr mASDescriptorPool;
		fre::VulkanDescriptorSetLayoutPtr mASDescriptorSetLayout;
		fre::VulkanDescriptorSetPtr mASDescriptorSet;
		fre::VulkanDescriptorPool mResultDP;
		fre::VulkanDescriptorSetLayout mResultDSL;
		fre::VulkanDescriptorSet mResultDS;
		fre::MeshPtr mResultMesh;

        uint32_t mRTShaderId = MAX(uint32_t);

		fre::MeshPtr mMesh;
		fre::MeshModelPtr mMeshModel;
		fre::VulkanBuffer mVertexBuffer;
		fre::VulkanBuffer mIndexBuffer;
		fre::VulkanBuffer mTransformMatrixBuffer;
		fre::AccelerationStructure mBLAS;
		fre::AccelerationStructure mTLAS;
		fre::VulkanBuffer mSceneGPU;
		fre::VulkanDescriptorPtr mTLASDescriptor;
		fre::VulkanDescriptorPtr mStorageImageDescriptor;
		fre::VulkanDescriptorPtr mRTMeshesGPUDescriptor;
		fre::VulkanDescriptorPtr mRTMaterialsGPUDescriptor;
		fre::VulkanDescriptorPtr mRTCameraDescriptor;
		fre::VulkanDescriptorPtr mRTTexturesDescriptor;

		VkPushConstantRange mCameraMatricesPCR;
		std::vector<fre::MeshPtr> mRTMeshes;
		fre::VulkanBuffer mRTMeshesGPUBuffer;
		fre::VulkanBuffer mRTMaterialsGPUBuffer;
		fre::VulkanBuffer mRTCameraBuffer;
        std::vector<VkImageView> mTextureViews;
        std::vector<VkSampler> mTextureSamplers;

        fre::RTCamera mRTCamera;
	};
}