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
		AppRenderer(fre::ThreadPool& threadPool);

		virtual void initUI() override;
		virtual int createDynamicGPUResources() override;
		virtual int createMeshGPUResources() override;

	protected:
		virtual void requestExtensions() override;
		virtual void requestDeviceFeatures() override;
		virtual bool isRayTracingSupported() { return true; }
        virtual void cleanupSwapChain() override;
        virtual void createSwapChain() override;
		virtual void update(const fre::Camera& camera, const fre::Light& light) override;

		virtual std::vector<const fre::VulkanShader*> getRTShaders(const uint32_t shaderId) override;
		virtual fre::ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) override;

	private:
		void createStorageImage();
		void loadMeshModel();
		void createResultMesh();
		void createAS();
		uint32_t createRTTexture(uint32_t textureId);
		void createSceneGPU();
		void createScene();
		void updateMaterials();

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

		std::vector<fre::MeshPtr> mRTMeshes;
		fre::VulkanBuffer mRTMeshesGPUBuffer;
		uint32_t mRTMaterialsGPUBufferIndex = MAX(uint32_t);
		fre::VulkanBuffer mRTMaterialsGPUBuffer;
        uint32_t mRTCameraBufferIndex = MAX(uint32_t);
        fre::VulkanBuffer mRTMeshesBuffer;
        fre::VulkanBuffer mRTMaterialsBuffer;
		fre::VulkanBuffer mRTCameraBuffer;
        std::vector<VkImageView> mTextureViews;
        std::vector<VkSampler> mTextureSamplers;
		uint32_t mShadowMissShaderId = MAX(uint32_t);

        fre::RTCamera mRTCamera;
	};
}