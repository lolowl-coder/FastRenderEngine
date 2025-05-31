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

	protected:
		virtual void requestExtensions() override;
		virtual void requestDeviceFeatures() override;
		virtual bool isRayTracingSupported() { return true; }
        virtual void cleanupSwapChain() override;
        virtual void createSwapChain() override;

		virtual fre::ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) override;

	private:
		void createSorageImage();
		void createStorageImageDP();
		void createStorageImageDSL();
		void allocateStorageImageDS();
		void loadMeshModel();
		void createAS();
		void createScene();

	private:
        fre::VulkanImage mStorageImage;
		fre::VulkanDescriptorPoolPtr mStorageImageDP;
		fre::VulkanDescriptorSetLayoutPtr mStorageImageDSL;
		fre::VulkanDescriptorSetPtr mStorageImageDS;
		fre::VulkanDescriptorPoolPtr mASDescriptorPool;
		fre::VulkanDescriptorSetLayoutPtr mASDescriptorSetLayout;
		fre::VulkanDescriptorSetPtr mASDescriptorSet;
		fre::VulkanDescriptorPool mResultDP;
		fre::VulkanDescriptorSetLayout mResultDSL;
		fre::VulkanDescriptorSet mResultDS;

        uint32_t mRTShaderId = MAX(uint32_t);

		fre::MeshPtr mMesh;
		fre::MeshModelPtr mMeshModel;
		fre::VulkanBuffer mVertexBuffer;
		fre::VulkanBuffer mIndexBuffer;
		fre::VulkanBuffer mTransformMatrixBuffer;
		fre::AccelerationStructure mBLAS;
		fre::AccelerationStructure mTLAS;
		VulkanDescriptorPtr mTLASDescriptor;
		VulkanDescriptorPtr mStorageImageDescriptor;

		VkPushConstantRange mCameraMatricesPCR;
	};
}