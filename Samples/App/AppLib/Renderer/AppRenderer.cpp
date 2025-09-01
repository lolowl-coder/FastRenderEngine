#include "Renderer/AppRenderer.hpp"
#include "Renderer/FeatureMacro.hpp"
#include "Renderer/FeatureStorage.hpp"
#include "Renderer/VulkanDescriptor.hpp"
#include "Renderer/VulkanSampler.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanTexture.hpp"
#include "UI/UIUtilities.hpp"
#include "Camera.hpp"
#include "Utilities.hpp"

#include<memory>

using namespace fre;
using namespace glm;

namespace app
{
	AppRenderer::AppRenderer(ThreadPool& threadPool)
		: VulkanRenderer(threadPool)
	{
	}

	void AppRenderer::initUI()
	{
		/*addUIRenderCallback
		(
			[this]()
			{
				if(!mRTMeshes.empty() && mRTMeshes[0] != nullptr)
				{
					ImGui::Begin("Settings");
                    ImGui::PushItemWidth(150.0f);
					Material& mat0 = getMaterial(mRTMeshes[0]->getMaterialId());
					Material& mat1 = getMaterial(mRTMeshes[1]->getMaterialId());
					bool changed = false;
					changed |= sliderFloat(0.0f, 1.0f, "Fish 0 shininess", mat0.mShininess, "%.2f");
					changed |= sliderFloat(0.0f, 1.0f, "Fish 1 shininess", mat1.mShininess, "%.2f");

					if(changed)
					{
						updateMaterials();
					}
					ImGui::PopItemWidth();
					ImGui::End();
				}
			}
		);*/
	}

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
		if(getTexture(0) == nullptr)
		{
			createTexture(getTextureInfo(0));
		}
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

        mBufferManager.udpateBuffer(mainDevice.logicalDevice, mRTCameraBufferIndex, &mRTCamera, sizeof(mRTCamera));

        VulkanRenderer::update(camera, light);
	}

	std::vector<const VulkanShader*> AppRenderer::getRTShaders(const uint32_t shaderId)
	{
		std::vector<const VulkanShader*> result;
		const Shader* shader = getShader(shaderId);
		auto shadowMissShader = getShader(mShadowMissShaderId);
		if(shader != nullptr && shadowMissShader != nullptr)
		{
			result =
			{
				&shader->mRayGenShader,
				&shader->mRayMissShader,
                & shadowMissShader->mRayMissShader,
				&shader->mRayClosestHitShader
			};
		}
		else
		{
            LOG_ERROR("Either shadow miss or main shader not found");
		}

		return result;
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
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
        //mat4 sceneTransform = rotate(mat4(1.0f), glm::half_pi<float>(), vec3(1.0f, 0.0f, 0.0f));
		mMeshModel = createMeshModel("Models/pool2/scene.gltf",
			{
				aiTextureType_NORMALS, aiTextureType_BASE_COLOR, aiTextureType_METALNESS,
				aiTextureType_EMISSIVE, aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP
			}, mat4(1.0f));
		mMeshModel->setVisible(false);
        auto shaderId = addShader("rt");
        mShadowMissShaderId = addShader("shadow");

		mat4 sceneTransform = rotate(mat4(1.0f), glm::half_pi<float>(), vec3(1.0f, 0.0f, 0.0f));
		sceneTransform = translate(sceneTransform, vec3(-mSceneBoundingBox.getCenter()));
		//mat4 sceneTransform = mat4(1.0f);

        // Apply scene transform to the model
        mMeshModel->setModelMatrix(sceneTransform * mMeshModel->getModelMatrix());

        // Post-process the model
		const int meshCount = mMeshModel->getMeshCount();
		for(int i = 0; i < meshCount; i++)
		{
			auto& mesh = mMeshModel->getMesh(i);

            // Force shader to ray tracing shader
			auto& material = getMaterial(mesh->getMaterialId());
			material.mShaderFileName = "rt";
			material.mShaderId = shaderId;
			mRTMeshes.push_back(mesh);

            // Apply scene transform to each mesh
			mesh->setModelMatrix(sceneTransform * mesh->getModelMatrix());

			/*std::vector<
			for(int i = 0; i < mesh->getIndexCount(); i++)
			{
				auto index0 = *(((uint32_t*)mesh->getIndexData()) + i);
				mesh->getVertexData()
			mEmissives.push_back(*/
		}

		auto worldCenter = sceneTransform * vec4(mSceneBoundingBox.getCenter(), 1.0f);
		auto worldSize = sceneTransform * vec4(mSceneBoundingBox.getCenter() + mSceneBoundingBox.getSize(), 1.0f) - worldCenter;
		mSceneBoundingBox.mMax = worldCenter + worldSize;
		mSceneBoundingBox.mMin = worldCenter - worldSize;
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
		auto& defaultTexture = getTexture(0);
		auto& texture = getTexture(textureId);
		if(textureId >= mTextureViews.size())
		{
			mTextureViews.resize(textureId + 1, defaultTexture->mImageView);
		}
		mTextureViews[textureId] = texture->mImageView;
		auto& textureInfo = getTextureInfo(textureId);
		auto samplerIndex = createSampler({ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, VK_FALSE });
		auto sampler = getSampler(samplerIndex);
		if(textureId >= mTextureSamplers.size())
		{
			mTextureSamplers.resize(textureId + 1, getSampler(0));
		}
		mTextureSamplers[textureId] = sampler;

		return textureId;
	}

    void AppRenderer::updateMaterials()
    {
		std::vector<RTMaterialGPU> rtMaterialsGPU;
		for(auto& material : mMaterials)
		{
			RTMaterialGPU rtMaterialGPU;

            rtMaterialGPU.mBaseColorFactor = material.mBaseColorFactor;
            rtMaterialGPU.mMetallicFactor = material.mMetallicFactor;
            rtMaterialGPU.mRoughnessFactor = material.mRoughnessFactor;
            rtMaterialGPU.mNormalScale = material.mNormalScale;
            rtMaterialGPU.mOcclusionStrength = material.mOcclusionStrength;
            rtMaterialGPU.mEmissiveFactor = material.mEmissiveFactor;
			if(material.mTextureIds.find(aiTextureType_BASE_COLOR) != material.mTextureIds.end())
			{
				rtMaterialGPU.mBaseColorMap = material.mTextureIds[aiTextureType_BASE_COLOR];
			}
			else
			{
				rtMaterialGPU.mBaseColorMap = -1;
			}
			if(material.mTextureIds.find(aiTextureType_METALNESS) != material.mTextureIds.end())
			{
				rtMaterialGPU.mMetallicRoughnessMap = material.mTextureIds[aiTextureType_METALNESS];
			}
			else
			{
				rtMaterialGPU.mMetallicRoughnessMap = -1;
			}
			if(material.mTextureIds.find(aiTextureType_NORMALS) != material.mTextureIds.end())
			{
				rtMaterialGPU.mNormalMap = material.mTextureIds[aiTextureType_NORMALS];
			}
			else
			{
				rtMaterialGPU.mNormalMap = -1;
			}
			if(material.mTextureIds.find(aiTextureType_EMISSIVE) != material.mTextureIds.end())
			{
				rtMaterialGPU.mEmissiveMap = material.mTextureIds[aiTextureType_EMISSIVE];
			}
			else
			{
				rtMaterialGPU.mEmissiveMap = -1;
			}
			if(material.mTextureIds.find(aiTextureType_AMBIENT_OCCLUSION) != material.mTextureIds.end())
			{
				rtMaterialGPU.mOcclusionMap = material.mTextureIds[aiTextureType_AMBIENT_OCCLUSION];
			}
			else
			{
				rtMaterialGPU.mOcclusionMap = -1;
			}
			// If not set before
			if(rtMaterialGPU.mOcclusionMap == -1 && material.mTextureIds.find(aiTextureType_LIGHTMAP) != material.mTextureIds.end())
			{
				rtMaterialGPU.mOcclusionMap = material.mTextureIds[aiTextureType_LIGHTMAP];
			}
			else
			{
				rtMaterialGPU.mOcclusionMap = -1;
			}
			rtMaterialsGPU.emplace_back(rtMaterialGPU);
		}
		if(mRTMaterialsGPUBuffer.mBuffer == VK_NULL_HANDLE)
		{
			VkBufferUsageFlags bufferUsageFlags =
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			const VkMemoryPropertyFlags memoryFlags =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			mRTMaterialsGPUBufferIndex = createBuffer(bufferUsageFlags, memoryFlags, rtMaterialsGPU.data(), rtMaterialsGPU.size() * sizeof(RTMaterialGPU));
			mRTMaterialsGPUBuffer = *mBufferManager.getBuffer(mRTMaterialsGPUBufferIndex);
		}
		else
		{
            mBufferManager.udpateBuffer(mainDevice.logicalDevice, mRTMaterialsGPUBufferIndex,
				rtMaterialsGPU.data(), rtMaterialsGPU.size() * sizeof(RTMaterialGPU));
		}
    }	

	void AppRenderer::createSceneGPU()
	{
		// For each model that was created, we retrieved the address of buffers
		// used by them. So in the shader, we have direct access to the data
		std::vector<RTMeshGPU> rtMeshesGPU;

        mTextureManager.forEachTexture([&](const VulkanTexturePtr& texture)
            {
                createRTTexture(texture->mId);
            });

		for(auto& rtMeshCPU : mRTMeshes)
		{
			RTMeshGPU rtMeshGPU;
			rtMeshGPU.mVertices = getVertexBuffer(rtMeshCPU->getId())->mDeviceAddress;
			rtMeshGPU.mIndices = getIndexBuffer(rtMeshCPU->getId())->mDeviceAddress;
			rtMeshGPU.mMaterialIndex = rtMeshCPU->getMaterialId();
			rtMeshesGPU.emplace_back(rtMeshGPU);
		}
		VkBufferUsageFlags bufferUsageFlags = 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags memoryFlags =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        uint32_t bufferIndex = createBuffer(bufferUsageFlags, memoryFlags, rtMeshesGPU.data(), rtMeshesGPU.size() * sizeof(RTMeshGPU));
        mRTMeshesGPUBuffer = *mBufferManager.getBuffer(bufferIndex);
		updateMaterials();
		mRTCameraBufferIndex = createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryFlags, &mRTCamera, sizeof(RTCamera));
        mRTCameraBuffer = *mBufferManager.getBuffer(mRTCameraBufferIndex);
	}

	void AppRenderer::createAS()
	{
		//Create BLAS for each mesh
		std::vector<VkAccelerationStructureInstanceKHR> blasInstances;
        for(auto& mesh : mRTMeshes)
        {
            auto blasIndex = createBLAS(mesh);
			mat4 matrix = mesh->getModelMatrix();
			blasInstances.push_back(createBlasInstance(blasIndex, matrix));
        }
		
		createSceneGPU();

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

		mMeshModel->getMesh(0)->setDescriptors({ {mTLASDescriptor, mStorageImageDescriptor, mRTCameraDescriptor, mRTMeshesGPUDescriptor, mRTMaterialsGPUDescriptor, mRTTexturesDescriptor} });
		mMeshModel->setVisible(true);

		mResultMesh->setVisible(true);
	}
}