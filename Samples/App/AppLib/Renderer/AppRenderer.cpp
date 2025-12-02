#include "Interop/Interop.hpp"
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
#include "CudaUtilities.hpp"
#include "Defines.hpp"
#include "Options.hpp"
#include "Utilities.hpp"

#include<memory>

using namespace fre;
using namespace glm;

#define TRACK_CHANGES(f) if(f){ changed = true; }

namespace app
{
	AppRenderer::AppRenderer(ThreadPool& threadPool)
		: VulkanRenderer(threadPool)
		, mCudaBufferManager("CudaBufferManager")
	{
	}

	void AppRenderer::destroy()
	{
		VulkanRenderer::destroy();
		mDenoiser.finish();
		for(int i = 0; i < mCudaWaitSemaphores.size(); i++)
		{
			cudaDestroyExternalSemaphore(mCudaWaitSemaphores[i]);
		}
		for(int i = 0; i < mCudaSignalSemaphores.size(); i++)
		{
			cudaDestroyExternalSemaphore(mCudaSignalSemaphores[i]);
		}
	}

	void AppRenderer::initUI(Camera& camera)
	{
		addUIRenderCallback
		(
			[this, & camera]()
			{
				if(!mRTMeshes.empty() && mRTMeshes[0] != nullptr)
				{
					ImGui::Begin("Options");
                    ImGui::PushItemWidth(150.0f);
					bool materialsChanged = false;
					bool changed = false;
					static int selectedIndex = -1;

					//const std::string lightMatName = "light_mat";
					const std::string lightMatName = "light_mat";
					/*if(ImGui::TreeNodeEx("Materials", 0, "Materials"))
					{
						for(int i = 0; i < mMaterials.size(); i++)
						{
							Material& mat = mMaterials[i];
							if(!mat.mName.empty())
							{
								ImGuiTreeNodeFlags_ selected = i == selectedIndex ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
								if(ImGui::TreeNodeEx(mat.mName.c_str(), ImGuiTreeNodeFlags_Leaf | selected, mat.mName.c_str()))
								{
									if(ImGui::IsItemClicked())
									{
										if(selectedIndex > -1)
										{
											// Restore previously selected material properties
											if(mMaterials[selectedIndex].mName == lightMatName)
											{
												mMaterials[selectedIndex].mEmissiveFactor = vec3(6.154f);
											}
											else
											{
												mMaterials[selectedIndex].mBaseColorFactor = vec4(1.0f);
											}
										}
										if(selected == ImGuiTreeNodeFlags_Selected)
										{
											selectedIndex = -1;
											selected = ImGuiTreeNodeFlags_None;
										}
										else
										{
											selectedIndex = i;
											selected = ImGuiTreeNodeFlags_Selected;
										}
										materialsChanged = true;
										//if(mat.mName == "light_mat")
										if(mat.mName == lightMatName)
										{
											mat.mEmissiveFactor = selected == ImGuiTreeNodeFlags_Selected ? vec3(6.154f, 0.0f, 0.0f) : vec3(6.154, 6.154f, 6.154f);
										}
										else
										{
											mat.mBaseColorFactor = selected == ImGuiTreeNodeFlags_Selected ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : vec4(1.0f);
										}
									}

									ImGui::TreePop();
								}
							}
						}

						ImGui::TreePop();
					}*/
					if(ImGui::CollapsingHeader("Ray tracing", ImGuiTreeNodeFlags_DefaultOpen))
					{
						bool tmp = mDynamicData.mLightingSettins.z;
						TRACK_CHANGES(ImGui::Checkbox("Enable GI", &tmp));
						mDynamicData.mLightingSettins.z = tmp ? 1.0f : 0.0f;
						{
							bool tmp = mDynamicData.mLightingSettins.w;
							TRACK_CHANGES(ImGui::Checkbox("Enable Area lights", &tmp));
							mDynamicData.mLightingSettins.w = tmp ? 1.0f : 0.0f;
						}
						if(ImGui::Checkbox("Denoise", &mIsDenoiserEnabled))
						{
							changed = true;
							setHasExternalResources(mIsDenoiserEnabled);
						}

						//inputFloat(-10.0f, 10.0f, "Flow multiplier", mDynamicData.mFlowMultiplier);
						//ImGui::Checkbox("Temporal mode", &mTemporalMode);
						//TRACK_CHANGES(sliderFloat(1.0f, 16.0f, "Max ray depth", mDynamicData.mRTSettings.x, "%.0f"));
						TRACK_CHANGES(sliderFloat(1.0f, 64.0f, "GI samples", mDynamicData.mRTSettings.y, "%.0f"));
						TRACK_CHANGES(sliderFloat(1.0f, 64.0f, "Emissive samples", mDynamicData.mRTSettings.z, "%.0f"));
					}

					if(ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
					{
						TRACK_CHANGES(sliderFloat(0.0f, 100.0f, "Main ligth intensity", mDynamicData.mLightingSettins.x, "%.2f"));
						TRACK_CHANGES(sliderFloat(0.0f, 10.0f, "Ambient intensity", mDynamicData.mLightingSettins.y, "%.2f"));
						TRACK_CHANGES(sliderFloat(-20.0f, 20.0f, "Exposure", mDynamicData.mBackgroundColor.w, "%.1f"));
						auto& itr = std::find_if(mMaterials.begin(), mMaterials.end(), [&lightMatName](const Material& mat) { return mat.mName == lightMatName; });
						if(itr != mMaterials.end())
						{
							float intensity = itr->mEmissiveFactor.x;
							if(sliderFloat(0.0f, 1000.0f, "Lamp light intensity", intensity, "%.2f"))
							{
								itr->mEmissiveFactor = vec3(intensity);
								changed = true;
								materialsChanged = true;
							}
						}
						ImGui::Text("Main light position");
						TRACK_CHANGES(inputFloat(-1000.0f, 1000.0f, "x", mDynamicData.mLightPos.x));
						TRACK_CHANGES(inputFloat(-1000.0f, 1000.0f, "y", mDynamicData.mLightPos.y));
						TRACK_CHANGES(inputFloat(-1000.0f, 1000.0f, "z", mDynamicData.mLightPos.z));
						if(ImGui::Button("Copy from camera eye"))
						{
							const auto t = inverse(camera.mView) * vec4(vec3(0.0f), 1.0f);

							mDynamicData.mLightPos.x = t.x;
							mDynamicData.mLightPos.y = t.y;
							mDynamicData.mLightPos.z = t.z;
							changed = true;
						}
						TRACK_CHANGES(sliderFloat(0.0f, 1000.0f, "Main light radius", mDynamicData.mLightPos.w));
						//TRACK_CHANGES(sliderFloat(0.0f, 10.0f, "Lod distance ratio", mDynamicData.mRTSettings.w));
						TRACK_CHANGES(ImGui::ColorEdit3("Main Light color", (float*)&mDynamicData.mLightColor.x, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoAlpha));
						TRACK_CHANGES(sliderFloat(0.0f, 10.0f, "Normal scale", mDynamicData.mLightColor.w));
						{
							bool enabled = mDynamicData.mEnableToneMapping != 0;
							if(ImGui::Checkbox("Tone mapping", &enabled))
							{
								mDynamicData.mEnableToneMapping = enabled ? 1 : 0;
								changed = true;
							}
						}
					}

					if(ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
					{
						OPTIONS;
						TRACK_CHANGES(sliderFloat(0.00001f, 10.0f, "Pan speed", options.mCameraPanSpeed, "%.7f"));
						TRACK_CHANGES(sliderFloat(0.00001f, 10.0f, "Zoom speed", options.mCameraZoomSpeed, "%.7f"));
					}
					if(ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen))
					{
						TRACK_CHANGES(ImGui::Combo("Background", &mDynamicData.mBackgroundType, "Static color\0HDR environment map\0"));
						if(mDynamicData.mBackgroundType == 0)
						{
							TRACK_CHANGES(ImGui::ColorEdit3("Background color", (float*)&mDynamicData.mBackgroundColor.x, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoAlpha));
							TRACK_CHANGES(sliderFloat(0.0f, 1000.0f, "Noise frequency", mDynamicData.mNoiseParams.x, "%.1f"));
							TRACK_CHANGES(sliderFloat(-10.0f, 100.0f, "Noise power", mDynamicData.mNoiseParams.y, "%.1f"));
							TRACK_CHANGES(sliderFloat(-10.0f, 10.0f, "Noise offset", mDynamicData.mNoiseParams.z, "%.1f"));
							TRACK_CHANGES(sliderFloat(-10.0f, 10.0f, "Noise amplitude", mDynamicData.mNoiseParams.w, "%.1f"));
						}
						else
						{
							const char* items[] = {
								"kloofendal_48d_partly_cloudy_puresky_4k.hdr",
								"modern_evening_street_4k.hdr",
								"moonless_golf_4k.hdr",
								"moonlit_golf_4k.hdr",
								"red_hill_straight_8k.hdr",
								"rogland_clear_night_4k.hdr",
								"rural_evening_road_4k.hdr",
								"sandsloot_4k.hdr"
							};
							static int itemIndex = 6;
							if(ImGui::Combo("File name", &itemIndex, items, IM_ARRAYSIZE(items)))
							{
								changed = true;

								mTextureFuture = std::async(std::launch::async,
									[this, items]()
								{
									VulkanTextureInfoPtr envTextureInfo = std::make_shared<VulkanTextureInfo>();
									*envTextureInfo = *getTextureInfo(mEnvTexIndex);
									envTextureInfo->mImage.mFileName = items[itemIndex];
									envTextureInfo->mImage.load();
									return envTextureInfo;
								});
							}
							if(mTextureFuture.valid() && mTextureFuture.wait_for(std::chrono::duration(std::chrono::milliseconds(0))) == std::future_status::ready)
							{
								VulkanTextureInfoPtr envTextureInfo = mTextureFuture.get();
								updateTextureImage(envTextureInfo);
								createRTTexture(mEnvTexIndex);
								updateTextureDescriptors();
							}
						}
						TRACK_CHANGES(sliderFloat(0.0f, 10.0f, "Firefly threshold", mDynamicData.mFireflyThreshold, "%.2f"));
						TRACK_CHANGES(sliderInt(1, 16, "AA samples", mDynamicData.mAASamples));
						TRACK_CHANGES(ImGui::Combo("Debug mode", &mDynamicData.mDebugMode, "Off\0Normals\0Albedo\0Roughness\0Metallic\0Emissive\0Ray distance\0Flow\0"));
					}
					ImGui::Separator();
					if(ImGui::Button("Restore defaults"))
					{
						selectedIndex = -1;
						mDynamicData = DynamicData();
						mMaterials = mDefaultMaterials;
                        changed = true;
						materialsChanged = true;
					}
                    ImGui::Separator();
                    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
					ImGui::Text("Camera position: %.2f, %.2f, %.2f", camera.mEye.x, camera.mEye.y, camera.mEye.z);
					
					//Progress bar for loaded textures
					if(mProgress < 1.0f)
					{
						std::lock_guard<std::mutex> lock(mMutex);
						ImGui::ProgressBar(mProgress, ImVec2(-FLT_MIN, 0), mProgressTextOverlay.c_str());
					}

					if(materialsChanged)
					{
						updateMaterials();
					}

					if(changed)
					{
						mAccumulatedFrames = 0;
					}
					ImGui::PopItemWidth();
					ImGui::End();
				}
			}
		);
	}

	void AppRenderer::onImageLoaded(const uint32_t imageIndex, const int loadQueueSize)
	{
		VulkanRenderer::onImageLoaded(imageIndex, loadQueueSize);
		mLoadedImageCount++;
		std::lock_guard lock(mMutex);
		mProgressTextOverlay = getTextureInfo(imageIndex)->mImage.mFileName;
		mProgress = static_cast<float>(mLoadedImageCount) / static_cast<float>(loadQueueSize - 1);
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

		REQUEST_FEATURE(
			VkPhysicalDeviceRobustness2FeaturesEXT,
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
			nullDescriptor);
	}

	void AppRenderer::cleanupSwapChain()
	{
		VulkanRenderer::cleanupSwapChain();
	}

	void AppRenderer::createSwapChain()
	{
        VulkanRenderer::createSwapChain();
	}

	void AppRenderer::updateTextureDescriptors()
	{
		mRTTexturesDescriptor = std::make_shared<DescriptorImage>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mTextureViews, mTextureSamplers);
		mMeshModel->getMesh(0)->setDescriptors({ {
			mTLASDescriptor,
			mColorStorage[mCurrentFrame].descriptor,
			mAlbedoStorage[mCurrentFrame].descriptor,
			mNormalStorage[mCurrentFrame].descriptor,
			mFlowStorage[mCurrentFrame].descriptor,
			mDynamicDataDescriptor,
			mRTMeshesGPUDescriptor,
			mRTMaterialsGPUDescriptor,
			mRTTexturesDescriptor,
			mEmissiveTrianglesDescriptor} });
	}

	void AppRenderer::update(const Camera& camera, const Light& light)
	{
		if(!mAllTexturesCreated)
		{
			std::vector<uint32_t> texturesToCreate;
			if(getTexture(0) == nullptr)
			{
				texturesToCreate.push_back(0);
			}
			if(getTexture(mEnvTexIndex) == nullptr)
			{
				texturesToCreate.push_back(mEnvTexIndex);
			}
			for(const auto& mesh : mRTMeshes)
			{
				const auto& material = getMaterial(mesh->getMaterialId());
				for(auto textureIndexPair : material.mTextureIds)
				{
					texturesToCreate.push_back(textureIndexPair.second);
				}
			}
			bool allTexturesCreated = true;
			for(auto texId : texturesToCreate)
			{
				auto& textureInfo = getTextureInfo(texId);
				if(textureInfo->mImage.mData != nullptr)
				{
					if(getTexture(textureInfo->mId) == nullptr)
					{
						createTexture(textureInfo);
						createRTTexture(texId);
						updateTextureDescriptors();
					}
				}
				else
				{
					allTexturesCreated = false;
				}
			}

			mAllTexturesCreated = allTexturesCreated;
        }

		mDynamicData.mViewInverse = glm::inverse(camera.mView);
		mDynamicData.mProjInverse = glm::inverse(camera.mProjection);
		mDynamicData.mPrevPV = camera.mProjection * camera.mPrevView;
		mDynamicData.mPV = camera.mProjection * camera.mView;

		if(mDynamicData.mPrevPV == mDynamicData.mPV && !mIsDenoiserEnabled)
		{
			mAccumulatedFrames++;
		}
		else
		{
			mAccumulatedFrames = 1;
		}
		mDynamicData.mFrameIndex = mAccumulatedFrames;
		mDynamicData.mEnvTexIndex = mEnvTexIndex;

        mBufferManager.udpateBuffer(mainDevice.logicalDevice, mDynamicDataBufferIndex, &mDynamicData, sizeof(mDynamicData));

		
        mResultMesh->setDescriptors({ { mColorStorage[mCurrentFrame].descriptor, mDynamicDataDescriptor } });
		mMeshModel->getMesh(0)->setDescriptors({ {
                mTLASDescriptor,
				mColorStorage[mCurrentFrame].descriptor,
				mAlbedoStorage[mCurrentFrame].descriptor,
				mNormalStorage[mCurrentFrame].descriptor,
				mFlowStorage[mCurrentFrame].descriptor,
				mDynamicDataDescriptor,
				mRTMeshesGPUDescriptor,
				mRTMaterialsGPUDescriptor,
				mRTTexturesDescriptor,
				mEmissiveTrianglesDescriptor} });

        VulkanRenderer::update(camera, light);
	}

	void AppRenderer::onRaytracingCommandsSubmitted()
	{
		VulkanRenderer::onRaytracingCommandsSubmitted();
		if(
			!mCUDAExternalColorBuffer.empty() &&
			!mCUDAExternalAlbedoBuffer.empty() &&
			!mCUDAExternalNormalBuffer.empty() &&
			mIsDenoiserEnabled)
		{
			cudaExternalSemaphoreWaitParams waitParams = {};
			waitParams.flags = 0;
			waitParams.params.fence.value = 0;
			// Wait for vulkan to complete it's work

			if(mHasExternalResources)
			{
				CUDA_CHECK(cudaWaitExternalSemaphoresAsync(&mCudaWaitSemaphores[mCurrentFrame], &waitParams, 1, 0));
			}

			Denoiser::Data data;
			auto maxViewSize = getViewport().getSize();
			data.width = maxViewSize.x;
			data.height = maxViewSize.y;
			data.color = reinterpret_cast<float*>(mCUDAExternalColorBuffer[mCurrentFrame].mData);
			data.albedo = reinterpret_cast<float*>(mCUDAExternalAlbedoBuffer[mCurrentFrame].mData);
			data.normal = reinterpret_cast<float*>(mCUDAExternalNormalBuffer[mCurrentFrame].mData);
			if(mTemporalMode)
			{
				data.flow = reinterpret_cast<float*>(mCUDAExternalFlowBuffer[mCurrentFrame].mData);
			}
			data.outputs.push_back(data.color);
			//data.flow = reinterpret_cast<float*>(flow.data);
			//data.flowtrust = reinterpret_cast<float*>(flowtrust.data);

			if(!mDenoiserInitialized)
			{
				const int tileWidth = 0;
				const int tileHeight = 0;
				const bool kpMode = mTemporalMode;
				const bool temporalMode = mTemporalMode;
				const bool applyFlow = false;
				const bool upscale2x = false;
				const OptixDenoiserAlphaMode alphaMode = OPTIX_DENOISER_ALPHA_MODE_COPY;
				const bool specularMode = false;
				mDenoiser.init(data, tileWidth, tileHeight, kpMode, temporalMode, applyFlow,
					upscale2x, alphaMode, specularMode);
				mDenoiserInitialized = true;
				setHasExternalResources(true);
			}
			else
			{
				mDenoiser.setTemporalMode(mTemporalMode);
				mDenoiser.update(data);
			}
			mDenoiser.exec();
			mDenoiser.getResults();

			cudaExternalSemaphoreSignalParams signalParams = {};
			signalParams.flags = 0;
			signalParams.params.fence.value = 0;

			// Signal vulkan to continue with the updated buffers
			CUDA_CHECK(cudaSignalExternalSemaphoresAsync(&mCudaSignalSemaphores[mCurrentFrame], &signalParams, 1, 0));
		}
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
                &shadowMissShader->mRayMissShader,
				&shader->mRayClosestHitShader,
				&shader->mRayAnyHitShader
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

    AppRenderer::StorageImage AppRenderer::createStorageImage(bool external, VkFormat format,VkImageTiling tiling, const std::string& name)
    {
		StorageImage result;

		auto maxViewSize = getViewport().getSize();
		Image image;
		image.mDimension = maxViewSize;
		image.mFormat = format;
		image.mFileName = name;
		image.mIsExternal = external;
		auto textureInfoId = mTextureManager.createTextureInfo(
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			tiling,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			image, 1u);
		auto textureInfo = getTextureInfo(textureInfoId);
		textureInfo->mImage.load();
		auto textureId = mTextureManager.createTexture(
			mainDevice,
			mTransferQueueFamilyId,
            mGraphicsQueueFamilyId,
			mGraphicsQueue,
			mGraphicsCommandPool,
            textureInfo);
		result.texture = getTexture(textureId);

		auto samplerId = createSampler({});
		auto sampler = getSampler(samplerId);

        std::vector<VkImageView> imageViews = { result.texture->mImageView };
        std::vector<VkSampler> samplers = { sampler };

		result.descriptor = std::make_shared<DescriptorImage>(
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL,
			imageViews, samplers);

		return result;
    }

	int AppRenderer::createDynamicGPUResources()
	{
        int result = VulkanRenderer::createDynamicGPUResources();

		if(result == 0)
		{
			//Create G-buffer
			for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				mColorStorage.push_back(createStorageImage(true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_LINEAR, formatString("#colorStorage%i", i)));
				mAlbedoStorage.push_back(createStorageImage(true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_LINEAR, formatString("#albedoStorage%i", i)));
				mNormalStorage.push_back(createStorageImage(true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_LINEAR, formatString("#normalStorage%i", i)));
				mFlowStorage.push_back(createStorageImage(true, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_LINEAR, formatString("#flowStorage%i", i)));
			}
		}

		return result;
	}

	void AppRenderer::loadMeshModel()
	{
		//mMeshModel = createMeshModel("Models/unitQuad/unitQuad.obj", {});
		//mMeshModel = createMeshModel("Models/unitCube/unitCube.obj", {});
        //mat4 sceneTransform = rotate(mat4(1.0f), glm::half_pi<float>(), vec3(1.0f, 0.0f, 0.0f));
		mMeshModel = createMeshModel("Models/pool2/scene.gltf",
		//mMeshModel = createMeshModel("Models/pool/scene.gltf",
		//mMeshModel = createMeshModel("Models/unreal/scene.gltf",
		//mMeshModel = createMeshModel("Models/mobileHome/scene.gltf",
		//mMeshModel = createMeshModel("Models/dreadroamer/scene.gltf",
		//mMeshModel = createMeshModel("Models/helmet/scene.gltf",
		//mMeshModel = createMeshModel("Models/sidewalk/scene.gltf",
		//mMeshModel = createMeshModel("Models/vipers_helmet/scene.gltf",
		//mMeshModel = createMeshModel("Models/sponza_palace/scene.gltf",
			{
				aiTextureType_NORMALS, aiTextureType_BASE_COLOR, aiTextureType_METALNESS,
				aiTextureType_EMISSIVE, aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP
			}, mat4(1.0f));
		mMeshModel->setVisible(false);
        auto shaderId = addShader("rt");
        mShadowMissShaderId = addShader("shadow");

		mat4 sceneTransform = rotate(mat4(1.0f), glm::half_pi<float>(), vec3(1.0f, 0.0f, 0.0f));
		//mat4 sceneTransform = mat4(1.0f);
		sceneTransform = translate(sceneTransform, vec3(-mSceneBoundingBox.getCenter()));
		//mat4 sceneTransform = mat4(1.0f);

        // Apply scene transform to the model
        mMeshModel->setModelMatrix(sceneTransform * mMeshModel->getModelMatrix());

		std::vector<EmissiveTri> emissives;

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
			mat4 modelMatrix = sceneTransform * mesh->getModelMatrix();
			mesh->setModelMatrix(modelMatrix);

			if(length(material.mEmissiveFactor) > 0.01 && mesh->getIndexCount() > 0)
			{
				LOG_INFO("Mesh {} is emissive", mesh->getName());
				for(int i = 0; i < mesh->getIndexCount(); i += 3)
				{
					auto index0 = *(((uint32_t*)mesh->getIndexData()) + i);
					auto index1 = *(((uint32_t*)mesh->getIndexData()) + i + 1);
					auto index2 = *(((uint32_t*)mesh->getIndexData()) + i + 2);
					const Vertex& vertex0 = *(static_cast<const Vertex*>(mesh->getVertexData()) + index0);
					const Vertex& vertex1 = *(static_cast<const Vertex*>(mesh->getVertexData()) + index1);
					const Vertex& vertex2 = *(static_cast<const Vertex*>(mesh->getVertexData()) + index2);
					const vec3 p0 = modelMatrix * vec4(vertex0.pos, 1.0);
					const vec3 p1 = modelMatrix * vec4(vertex1.pos, 1.0);
					const vec3 p2 = modelMatrix * vec4(vertex2.pos, 1.0);
					const vec2 uv0 = vertex0.tex;
					const vec2 uv1 = vertex1.tex;
					const vec2 uv2 = vertex2.tex;
					EmissiveTri emissiveTri = {p0, p1, p2, uv0, uv1, uv2, length(cross(p1 - p0, p2 - p0)), material.mId};
                    emissives.push_back(emissiveTri);
				}
			}
		}

		if(!emissives.empty())
		{
			auto emissivesBufferIndex = createBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				emissives.data(), sizeof(EmissiveTri) * emissives.size());
			mEmissiveTrianglesBuffer = *mBufferManager.getBuffer(emissivesBufferIndex);
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
		mResultMesh->setVisible(false);
		addMeshModel({ mResultMesh });
	}

	void AppRenderer::loadEnvTexture()
	{
		Image image;
        image.mFileName = "rural_evening_road_4k.hdr";
		image.mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		mEnvTexIndex = createTextureInfo(
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			image, 1u);
	}

	int AppRenderer::createMeshGPUResources()
	{
		loadEnvTexture();
		loadMeshModel();

		int result = VulkanRenderer::createMeshGPUResources();

		mTextureManager.forEachTextureInfo(
			[this](const VulkanTextureInfoPtr& textureInfo)
			{
				createRTTexture(textureInfo->mId);
			}
		);

		createResultMesh();

		createScene();
		
		return result;
	}

	uint32_t AppRenderer::createRTTexture(uint32_t textureId)
	{
		// If texture is found, we use it, otherwise we use the default one (id 0) for textureId slot.
		// Later it is updated with actual texture when it is created.
		auto srcTextureId = 0;
		auto texture = getTexture(textureId);
		if(texture == nullptr)
		{
			texture = getTexture(0);
		}
		else
		{
			srcTextureId = textureId;
		}

		if(textureId >= mTextureViews.size())
		{
			mTextureViews.resize(textureId + 1, texture->mImageView);
		}
		else
		{
			mTextureViews[textureId] = texture->mImageView;
		}

		// Here default texture can be used with its sampler
		auto& textureInfo = getTextureInfo(srcTextureId);
		auto samplerIndex = createSampler(
			{
				VK_SAMPLER_ADDRESS_MODE_REPEAT,
				srcTextureId == mEnvTexIndex ? VK_FILTER_NEAREST : VK_FILTER_LINEAR,
				VK_FALSE, textureInfo->mMipLevelCount
			});
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
		mDynamicDataBufferIndex = createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryFlags, &mDynamicData, sizeof(DynamicData));
        mDynamicDataBuffer = *mBufferManager.getBuffer(mDynamicDataBufferIndex);
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

	void AppRenderer::initInterop()
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			mCUDAExternalColorBuffer.push_back(mCudaBufferManager.createExternalBuffer<float4>(mColorStorage[i].texture->mId, this));
			mCUDAExternalAlbedoBuffer.push_back(mCudaBufferManager.createExternalBuffer<float4>(mAlbedoStorage[i].texture->mId, this));
			mCUDAExternalNormalBuffer.push_back(mCudaBufferManager.createExternalBuffer<float4>(mNormalStorage[i].texture->mId, this));
			mCUDAExternalFlowBuffer.push_back(mCudaBufferManager.createExternalBuffer<float4>(mFlowStorage[i].texture->mId, this));
		}
        assert(!mExternalWaitSemaphores.empty());
        assert(!mExternalSignalSemaphores.empty());

		mCudaSignalSemaphores.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
		mCudaWaitSemaphores.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
		for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			//Vulkan will wait for cuda's signal
			importCudaExternalSemaphore(mCudaSignalSemaphores[i], mExternalWaitSemaphores[i],
				getDefaultSemaphoreHandleType(), this);

			//Cuda will wait for Vulkan's signal
			importCudaExternalSemaphore(mCudaWaitSemaphores[i], mExternalSignalSemaphores[i],
				getDefaultSemaphoreHandleType(), this);
		}
	}

	void AppRenderer::createScene()
	{
		createAS();

		mTLASDescriptor = std::make_shared<DescriptorAccelerationStructure>(mTLAS.mHandle);
		mDynamicDataDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mDynamicDataBuffer.mBuffer);
		mRTMeshesGPUDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mRTMeshesGPUBuffer.mBuffer);
		mRTMaterialsGPUDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mRTMaterialsGPUBuffer.mBuffer);
 		mRTTexturesDescriptor = std::make_shared<DescriptorImage>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mTextureViews, mTextureSamplers);
		mEmissiveTrianglesDescriptor = std::make_shared<DescriptorBuffer>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mEmissiveTrianglesBuffer.mBuffer);
		mMeshModel->setVisible(true);
        
		mResultMesh->setVisible(true);
        
		initInterop();
		mDefaultMaterials = mMaterials;
	}
}