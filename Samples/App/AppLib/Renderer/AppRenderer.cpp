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
		//addDeviceExtension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

		// Required by VK_KHR_spirv_1_4
		addDeviceExtension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	}

	void AppRenderer::requestDeviceFeatures()
	{
		VulkanRenderer::requestDeviceFeatures();

		mDeviceFeatures.features.shaderInt64 = VK_TRUE;

		mDeviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		mDeviceFeatures12.runtimeDescriptorArray = VK_TRUE;
		mDeviceFeatures12.descriptorBindingPartiallyBound = VK_TRUE;
		mDeviceFeatures12.descriptorBindingVariableDescriptorCount = VK_TRUE;
		mDeviceFeatures12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		mDeviceFeatures12.bufferDeviceAddress = VK_TRUE;
		mDeviceFeatures12.scalarBlockLayout = VK_TRUE;

		mDeviceFeatures.pNext = &mDeviceFeatures12;

		mLastDeviceFeatures = &mDeviceFeatures12.pNext;

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

	void AppRenderer::update(const Camera& camera, const Light& light)
	{
		if(mTLAS.mHandle == VK_NULL_HANDLE)
		{
			bool allTexturesCreated = true;
			for(const auto& mesh : mRTMeshes)
			{
				const auto& material = getMaterial(mesh->getMaterialId());
				for(auto textureIndexPair : material.mTextureIds)
				{
					auto& textureInfo = getTextureInfo(textureIndexPair.second);
					if(textureInfo->mImage.mData != nullptr)
					{
						if(getTexture(textureInfo->mId) == nullptr)
						{
							createTexture(textureInfo);
						}
					}
					else
					{
						allTexturesCreated = false;
					}
				}
			}
            if(allTexturesCreated)
            {
                createScene();
            }
        }

		mRTCamera.mViewInverse = glm::inverse(camera.mView);
		mRTCamera.mProjInverse = glm::inverse(camera.mProjection);

        VulkanRenderer::update(camera, light);
	}
	
	ShaderMetaDatum AppRenderer::getShaderMetaData(const std::string& shaderFileName)
	{
		ShaderMetaDatum result = VulkanRenderer::getShaderMetaData(shaderFileName);
		if(result.empty())
		{
			if(shaderFileName == "rt")
			{
				ShaderMetaData md;
				md.mDepthTestEnabled = true;
				md.mSubPassIndex = 0;

				result.push_back(md);
			}
			else if(shaderFileName == "renderStorageImage")
			{
				ShaderMetaData md;
				md.mDepthTestEnabled = false;
				md.mVertexSize = 0;
				md.mSubPassIndex = 0;

				result.push_back(md);
			}
		}

		return result;
	}

    void AppRenderer::createStorageImage()
    {
		auto maxViewSize = getViewport().getSize();
		Image image;
		image.mDimension = maxViewSize;
		image.mFormat = VK_FORMAT_R8G8B8A8_UNORM;
		auto textureInfoId = mTextureManager.createTextureInfo(
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			false,
			image);
		auto textureInfo = getTextureInfo(textureInfoId);
		auto textureId = mTextureManager.createTexture(
			mainDevice,
			mTransferQueueFamilyId,
            mGraphicsQueueFamilyId,
			mGraphicsQueue,
			mGraphicsCommandPool,
            textureInfo);
		mStorageImage = getTexture(textureId);

		auto samplerId = createSampler({});
		auto sampler = getSampler(samplerId);

        std::vector<VkImageView> imageViews = { mStorageImage->mImageView };
        std::vector<VkSampler> samplers = { sampler };

		mStorageImageDescriptor = std::make_shared<DescriptorImage>(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL,
			imageViews, samplers);
    }

	int AppRenderer::createDynamicGPUResources()
	{
        int result = VulkanRenderer::createDynamicGPUResources();

		if(result == 0)
		{
			createStorageImage();
		}

		return result;
	}

	void AppRenderer::loadMeshModel()
	{
		//mMeshModel = createMeshModel("Models/unitQuad/unitQuad.obj", {});
		//mMeshModel = createMeshModel("Models/unitCube/unitCube.obj", {});
		mMeshModel = createMeshModel("Models/fish/scene.gltf", { aiTextureType_NORMALS, aiTextureType_BASE_COLOR });
		mMeshModel->setVisible(false);
        mMesh = mMeshModel->getMesh(0);
		Material& material = getMaterial(mMesh->getMaterialId());
        auto shaderId = addShader("rt");
		material.mShaderFileName = "rt";
        material.mShaderId = shaderId;
		//material.mShininess = 1.0f;

		//Create instances of the same mesh with different BASE_COLOR texture
		auto mesh0 = std::make_shared<Mesh>();
		*mesh0 = *mMesh;
		auto& material0 = getMaterial(mMesh->getMaterialId());
		auto mesh1 = std::make_shared<Mesh>();
        //Create a new material with the same shader and different texture
		Material material1;
		material1.mShaderFileName = material0.mShaderFileName;
		material1.mShaderId = material0.mShaderId;
		material1.mShininess = material0.mShininess;
		material1.mTextureIds = material0.mTextureIds;
		//Create texture for second material instance
		auto& texInfo0 = getTextureInfo(material0.mId);
		Image image1;
		image1.mFileName = "Textures/test.png";
		auto textureInfoIndex1 = createTextureInfo(texInfo0->mAddressMode, texInfo0->mTiling, texInfo0->mUsageFlags, texInfo0->mMemoryFlags, texInfo0->mLayout, false, image1);
        //Update texture id in the material
		material1.mTextureIds[aiTextureType_BASE_COLOR] = textureInfoIndex1;
		*mesh1 = *mMesh;
		mesh1->setMaterialId(material1.mId);
        //Add materials to the renderer
        addMaterial(material1);

		//Collect RT meshes
		mRTMeshes.push_back(mesh0);
		mRTMeshes.push_back(mesh1);
	}
	
	void AppRenderer::createResultMesh()
	{
		Material material;
		material.mShaderFileName = "renderStorageImage";
		addMaterial(material);
		mResultMesh = std::make_shared<Mesh>(material.mId);
		mResultMesh->setGeneratedVerticesCount(3);
        mResultMesh->setDescriptors({ { mStorageImageDescriptor } });
		mResultMesh->setVisible(false);
		addMeshModel({ mResultMesh });
	}

	int AppRenderer::createMeshGPUResources()
	{
		loadMeshModel();

		int result = VulkanRenderer::createMeshGPUResources();

		createResultMesh();
		
		return result;
	}

	uint32_t AppRenderer::createRTTexture(uint32_t textureId)
	{
        uint32_t result = mTextureViews.size();
		auto& texture = getTexture(textureId);
		mTextureViews.push_back(texture->mImageView);
		auto& textureInfo = getTextureInfo(textureId);
		auto samplerIndex = createSampler({ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, VK_FALSE });
		auto sampler = getSampler(samplerIndex);
		mTextureSamplers.push_back(sampler);

		return result;
	}

	void AppRenderer::createSceneGPU()
	{
		// For each model that was created, we retrieved the address of buffers
		// used by them. So in the shader, we have direct access to the data
		std::vector<RTMeshGPU> rtMeshesGPU;
		std::vector<RTMaterialGPU> rtMaterialsGPU;

		for(auto& rtMeshCPU : mRTMeshes)
		{
			RTMeshGPU rtMeshGPU;
			rtMeshGPU.mVertices = getVertexBuffer(rtMeshCPU->getId())->mDeviceAddress;
			rtMeshGPU.mIndices = getIndexBuffer(rtMeshCPU->getId())->mDeviceAddress;
            auto& material = getMaterial(rtMeshCPU->getMaterialId());
			rtMeshGPU.mMaterialIndex = rtMaterialsGPU.size();
			rtMeshesGPU.emplace_back(rtMeshGPU);

            RTMaterialGPU rtMaterialGPU;
            rtMaterialGPU.mDiffuseMap = createRTTexture(material.mTextureIds[aiTextureType_BASE_COLOR]);
            rtMaterialGPU.mNormalMap = createRTTexture(material.mTextureIds[aiTextureType_NORMALS]);
			//TODO fix specular
            rtMaterialGPU.mSpecular = mLighting.lightSpecularColor;
            rtMaterialGPU.mShininess = material.mShininess;
			rtMaterialsGPU.emplace_back(rtMaterialGPU);
		}
		VkBufferUsageFlags bufferUsageFlags = 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags memoryFlags =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		mRTMeshesGPUBuffer = createBuffer(bufferUsageFlags, memoryFlags, rtMeshesGPU.data(), rtMeshesGPU.size() * sizeof(RTMeshGPU));
		mRTMaterialsGPUBuffer = createBuffer(bufferUsageFlags, memoryFlags, rtMaterialsGPU.data(), rtMaterialsGPU.size() * sizeof(RTMaterialGPU));
		mRTCameraBuffer = createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryFlags, &mRTCamera, sizeof(RTCamera));
	}

	void AppRenderer::createAS()
	{
		//Create BLAS for each mesh
		auto blasIndex0 = createBLAS(mRTMeshes[0]);
		auto blasIndex1 = createBLAS(mRTMeshes[1]);

		mat4 matrix0 = mat4(1.0f);
		mat4 matrix1 = translate(mat4(1.0f), vec3(0.5f, 0.0f, 0.0f));

		createSceneGPU();

		std::vector<VkAccelerationStructureInstanceKHR> blasInstances =
		{
			createBlasInstance(blasIndex0, matrix0),
			createBlasInstance(blasIndex1, matrix1)
		};

		auto tlasIndex = createTLAS(blasInstances);
		mTLAS = getAS(tlasIndex);
	}

	void AppRenderer::createScene()
	{
		createAS();

		mTLASDescriptor = std::make_shared<DescriptorAccelerationStructure>(mTLAS.mHandle);
		mRTCameraDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mRTCameraBuffer.mBuffer);
		mRTMeshesGPUDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mRTMeshesGPUBuffer.mBuffer);
		mRTMaterialsGPUDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mRTMaterialsGPUBuffer.mBuffer);
		mRTTexturesDescriptor = std::make_shared<DescriptorImage>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mTextureViews, mTextureSamplers);

		mMesh->setDescriptors({ {mTLASDescriptor, mStorageImageDescriptor, mRTCameraDescriptor, mRTMeshesGPUDescriptor, mRTMaterialsGPUDescriptor, mRTTexturesDescriptor} });
		mMeshModel->setVisible(true);

		mResultMesh->setVisible(true);
	}
}