#include "Renderer/AppRenderer.hpp"
#include "Renderer/FeatureMacro.hpp"
#include "Renderer/FeatureStorage.hpp"
#include "Renderer/VulkanDescriptor.hpp"
#include "Renderer/VulkanSampler.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanTexture.hpp"
#include "Camera.hpp"
#include "Utilities.hpp"

#include<memory>

using namespace fre;
using namespace glm;

namespace app
{
	struct CameraMatrices
	{
		mat4 mViewInverse;
		mat4 mProjInverse;
	};

	void AppRenderer::requestExtensions()
	{
		VulkanRenderer::requestExtensions();

		// Ray tracing related extensions required by this sample
		addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

		// Required by VK_KHR_acceleration_structure
		addDeviceExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		addDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

		// Required for VK_KHR_ray_tracing_pipeline
		addDeviceExtension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

		// Required by VK_KHR_spirv_1_4
		addDeviceExtension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	}

	void AppRenderer::requestDeviceFeatures()
	{
		VulkanRenderer::requestDeviceFeatures();

		REQUEST_FEATURE(
			VkPhysicalDeviceBufferDeviceAddressFeatures,
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
			bufferDeviceAddress);

		REQUEST_FEATURE(
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR,
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
			rayTracingPipeline);

		REQUEST_FEATURE(
			VkPhysicalDeviceAccelerationStructureFeaturesKHR,
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
			accelerationStructure);
	}

	void AppRenderer::cleanupSwapChain()
	{
		VulkanRenderer::cleanupSwapChain();
	}

	void AppRenderer::createSwapChain()
	{
        VulkanRenderer::createSwapChain();
	}
	
	ShaderMetaDatum AppRenderer::getShaderMetaData(const std::string& shaderFileName)
	{
		ShaderMetaDatum result;
		if(result.empty())
		{
			if(shaderFileName == "rt")
			{
				ShaderMetaData md;
				md.mPushConstantRanges = { mCameraMatricesPCR };
				md.mPushConstantsCallback = [this](const Mesh::Ptr& mesh, const mat4& modelMatrix, const Camera& camera, const Light& light, VkPipelineLayout pipelineLayout, uint32_t instanceId)
					{
						if(mesh != nullptr)
						{
							CameraMatrices cameraMatrices = { camera.mView, camera.mProjection };
							pushConstants(mCameraMatricesPCR, &modelMatrix[0], pipelineLayout, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
						}
					};
				md.mDepthTestEnabled = true;
				md.mSubPassIndex = 0;

				result.push_back(md);
			}
		}

		return result;
	}

    void AppRenderer::createSorageImage()
    {
		auto maxViewSize = getViewport().getSize();
		auto textureInfoId = mTextureManager.createTextureInfo(
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			false,
			Image());
		auto textureInfo = getTextureInfo(textureInfoId);
		auto textureId = mTextureManager.createTexture(
			mainDevice,
			mTransferQueueFamilyId,
            mGraphicsQueueFamilyId,
			mGraphicsQueue,
			mGraphicsCommandPool,
            textureInfo);
		mStorageImage = getTexture(textureId);
    }

	void AppRenderer::loadMeshModel()
	{
		Material material;
		material.mShininess = 1.0f;
		mRTShaderId = addShader("rt");
		material.mShaderId = mRTShaderId;
		addMaterial(material);

		mMeshModel = createMeshModel("Models/unitQuad/unitQuad.obj", {});
        mMesh = mMeshModel->getMesh(0);
		mMesh->setMaterialId(material.mId);
		mTLASDescriptor = std::make_shared<DescriptorAccelerationStructure>(mTLAS.mHandle);
		auto samplerId = createSampler({});
		auto sampler = getSampler(samplerId);
		mStorageImageDescriptor = std::make_shared<DescriptorImage>(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL, mStorageImage->mImageView, sampler);
		mMesh->setDescriptors({{mTLASDescriptor}, {mStorageImageDescriptor}});
	}

    int AppRenderer::createCoreGPUResources(GLFWwindow* newWindow)
    {
        auto result = VulkanRenderer::createCoreGPUResources(newWindow);
        if(result == 0)
        {
			createSorageImage();
        }
        return result;
    }

	int AppRenderer::createDynamicGPUResources()
	{
        int result = VulkanRenderer::createDynamicGPUResources();

		if(result == 0)
		{
			createScene();
		}

		return result;
	}

	void AppRenderer::createAS()
	{
		// Setup vertices and indices for a single triangle
		struct Vertex
		{
			vec3 pos;
		};
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

        const fre::Vertex* meshVertices = static_cast<const fre::Vertex*>(mMesh->getVertexData());
        for(uint32_t i = 0; i < mMesh->getVertexCount(); i++)
        {
			vertices.push_back({meshVertices[i].pos});
        }

		const uint32_t* meshIndices = static_cast<const uint32_t*>(mMesh->getIndexData());
		for(uint32_t i = 0; i < mMesh->getIndexCount(); i++)
		{
			indices.push_back(meshIndices[i]);
		}

		auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
		auto index_buffer_size = indices.size() * sizeof(uint32_t);
		VkTransformMatrixKHR transform_matrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		// Create buffers for the bottom level geometry
		// For the sake of simplicity we won't stage the vertex data to the GPU memory

		// Note that the buffer usage flags for buffers consumed by the bottom level acceleration structure require special flags
		const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		mVertexBuffer = createBuffer(buffer_usage_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertices.data(), vertex_buffer_size);
		mIndexBuffer = createBuffer(buffer_usage_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indices.data(), index_buffer_size);

		// Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
		mTransformMatrixBuffer = createBuffer(
			buffer_usage_flags,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&transform_matrix, sizeof(transform_matrix));

		mBLAS = createBLAS(mVertexBuffer, mIndexBuffer, mTransformMatrixBuffer);
		mTLAS = createTLAS(mBLAS.mDeviceAddress, transform_matrix);
	}

	void AppRenderer::createScene()
	{
		createSorageImage();

		mCameraMatricesPCR.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		mCameraMatricesPCR.offset = 0;
		mCameraMatricesPCR.size = sizeof(CameraMatrices);

		loadMeshModel();
		createAS();
	}


}