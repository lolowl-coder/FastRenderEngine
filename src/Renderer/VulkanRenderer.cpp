#include <volk.h>

#include "Renderer/ShaderInputParser.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/FeatureStorage.hpp"
#include "Renderer/FeatureMacro.hpp"
#include "Renderer/VulkanDescriptor.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Renderer/VulkanSampler.hpp"

#include "FileSystem/FileSystem.hpp"
#include "VulkanAccelerationStructure.hpp"
#include "Camera.hpp"
#include "Log.hpp"
#include "ThreadPool.hpp"
#include "Timer.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanShader.hpp"
#include "config.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "implot.h"

#include <spdlog/fmt/bin_to_hex.h>

#include <limits>
#include <stdexcept>
#include <mutex>

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

//#define REDRAW_EVERY_FRAME

using namespace glm;

namespace fre
{
    std::mutex gRenderMutex;
	
	VulkanRenderer::VulkanRenderer(ThreadPool& threadPool)
	: mThreadPool(threadPool)
	{
		LOG_TRACE("Number of concurent threads: {}", std::thread::hardware_concurrency());
		Image image;
        image.mFileName = "default.jpg";
        createTextureInfo(
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			false, image);
	}

	VulkanRenderer::~VulkanRenderer()
	{
	}

	int VulkanRenderer::createCoreGPUResources(GLFWwindow* newWindow)
	{
		LOG_INFO("VulkanRenderer. Create core GPU resources");
		mWindow = newWindow;
		int result = 0;

		{
			requestExtensions();
			createInstance();
			createSurface();
			getPhysicalDevice();
			createLogicalDevice();
			if(isRayTracingSupported())
			{
				initRayTracing();
			}
			mSwapChain.create(mWindow, mainDevice, mGraphicsQueueFamilyId,
				mPresentationQueueFamilyId, mSurface);
			mRenderPass.create(mainDevice, mSwapChain.mSwapChainImageFormat);
			createSwapChainFrameBuffers();
			mTextureManager.create(mainDevice.logicalDevice);
			createSynchronisation();

			LOG_INFO("VulkanRenderer. Core GPU resources created");
		}

		return result;
	}

	int VulkanRenderer::createDynamicGPUResources()
	{
		LOG_INFO("VulkanRenderer. Create dynamic GPU resources");
		int result = 0;
		try
		{
			createInputDescriptorPool();
			createInputDescriptorSetLayout();
			allocateUniformDescriptorSets();
			allocateInputDescriptorSets();
			createCommandPools();
			createCommandBuffers();
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			result = EXIT_FAILURE;
		}
		catch(...)
		{
			LOG_ERROR("Unknown exception");
			result = EXIT_FAILURE;
		}

		LOG_INFO("VulkanRenderer. Dynamic GPU resources created");

		return result;
	}

	int VulkanRenderer::createMeshGPUResources()
	{
		LOG_INFO("VulkanRenderer. Create mesh GPU resources");
		int result = 0;
		try
		{
			loadMeshes();
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			result = EXIT_FAILURE;
		}

		LOG_INFO("VulkanRenderer. Mesh GPU resources created");

		return result;
	}

	int VulkanRenderer::createLoadableGPUResources()
	{
		LOG_INFO("VulkanRenderer. Create loadable GPU resources");
		int result = 0;
		try
		{
			loadUsedShaders();
			createPipelines();
			loadImages();

			createUI();
		}
		catch (std::runtime_error& e)
		{
			LOG_ERROR(e.what());
			result = EXIT_FAILURE;
		}

		LOG_INFO("VulkanRenderer. Loadable GPU resources created");

		return result;
	}

	int VulkanRenderer::postCreate()
	{
		return 0;
	}

    void VulkanRenderer::destroy()
    {
		//Wait until no actions being run on device before destroying
		if(mainDevice.logicalDevice != VK_NULL_HANDLE)
		{
			VK_CHECK(vkDeviceWaitIdle(mainDevice.logicalDevice));
		}
	}

	void VulkanRenderer::destroyGPUResources()
	{
		if(mainDevice.logicalDevice != VK_NULL_HANDLE)
		{
			cleanupRayTracing();

			cleanupUI();

			//_aligned_free(modetTransferSpace);

			mBufferManager.destroy(mainDevice.logicalDevice);

            int count = mDescriptorSetCache.size();
			for(int i = 0; i < count; i++)
			{
				auto& dp = mDescriptorPoolCache.getByIndex(i);
				dp->destroy(mainDevice.logicalDevice);
			}

			count = mDescriptorSetLayoutCache.size();
			for(int i = 0; i < count; i++)
			{
				auto& dsl = mDescriptorSetLayoutCache.getByIndex(i);
				dsl->destroy(mainDevice.logicalDevice);
			}

			count = mSamplerCache.size();
			for(int i = 0; i < count; i++)
			{
				auto& s = mSamplerCache.getByIndex(i);
				vkDestroySampler(mainDevice.logicalDevice, s, nullptr);
			}

			cleanupUniformDescriptorPool();
			cleanupInputDescriptorPool();
			cleanupUIDescriptorPool();

			cleanupInputDescriptorSetLayout();
			cleanupUniformDescriptorSetLayout();

			mTextureManager.destroy(mainDevice.logicalDevice);

			mSwapChain.destroy(mainDevice.logicalDevice);
			cleanupSwapChainFrameBuffers();

			/*for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
			{
				vkDestroyBuffer(mainDevice.logicalDevice, mVPUniformBuffer[i], nullptr);
				vkFreeMemory(mainDevice.logicalDevice, mVPUniformBufferMemory[i], nullptr);
			
				//vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
				//vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
			}*/

			cleanupSwapchainImagesSemaphores();
			cleanupRenderFinishedSemaphores();
			cleanupDrawFences();
			cleanupComputeFences();
			cleanupComputeFinishedSemaphores();
			cleanupTransferSynchronisation();
			cleanupSemaphores();

			vkDestroyCommandPool(mainDevice.logicalDevice, mGraphicsCommandPool, nullptr);
			if(mGraphicsCommandPool != mTransferCommandPool)
			{
				vkDestroyCommandPool(mainDevice.logicalDevice, mTransferCommandPool, nullptr);
			}
			if(mGraphicsCommandPool != mComputeCommandPool || mTransferCommandPool != mComputeCommandPool)
			{
				vkDestroyCommandPool(mainDevice.logicalDevice, mComputeCommandPool, nullptr);
			}
		
			cleanupPipelines(mainDevice.logicalDevice);

			mRenderPass.destroy(mainDevice.logicalDevice);
		
			vkDestroyDevice(mainDevice.logicalDevice, nullptr);
		}
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyInstance(mInstance, nullptr);
	}

	uint32_t VulkanRenderer::createDescriptorPool(const VulkanDescriptorPoolKey& key)
	{
        return mDescriptorPoolCache.findOrCreate(key, [this](const VulkanDescriptorPoolKey& key)
            {
                VulkanDescriptorPoolPtr dp = std::make_shared<VulkanDescriptorPool>();
                dp->create(mainDevice.logicalDevice, MAX_FRAME_DRAWS, key.mPoolSizes);
                return dp;
            });
	};

	VulkanDescriptorPoolPtr& VulkanRenderer::getDescriptorPool(const uint32_t index)
	{
		return mDescriptorPoolCache.getByIndex(index);
	}

	uint32_t VulkanRenderer::createDescriptorSetLayout(const VulkanDescriptorSetLayoutInfo& key)
	{
		return mDescriptorSetLayoutCache.findOrCreate(key, [this](const VulkanDescriptorSetLayoutInfo& key)
			{
				VulkanDescriptorSetLayoutPtr dsl = std::make_shared<VulkanDescriptorSetLayout>();
				dsl->create(mainDevice.logicalDevice, key);
				return dsl;
			});
	}

	VulkanDescriptorSetLayoutPtr& VulkanRenderer::getDescriptorSetLayout(const uint32_t index)
	{
		return mDescriptorSetLayoutCache.getByIndex(index);
	}

    uint32_t VulkanRenderer::createDescriptorSet(const VulkanDescriptorSetKey& key)
	{
		return mDescriptorSetCache.findOrCreate(key, [this](const VulkanDescriptorSetKey& key)
			{
                VulkanDescriptorPoolPtr dp = mDescriptorPoolCache.getByIndex(key.mDPId);
                VulkanDescriptorSetLayoutPtr dsl = mDescriptorSetLayoutCache.getByIndex(key.mDSLId);
				VulkanDescriptorSetPtr ds = std::make_shared<VulkanDescriptorSet>();
				ds->allocate(mainDevice.logicalDevice, dp->mDescriptorPool, dsl->mDescriptorSetLayout);
				return ds;
			});
	}

	VulkanDescriptorSetPtr& VulkanRenderer::getDescriptorSet(const uint32_t index)
	{
        return mDescriptorSetCache.getByIndex(index);
	}

	void VulkanRenderer::bindDescriptorSets(const std::vector<uint32_t>& setIds, VkPipelineLayout pipelineLayout, VkPipelineBindPoint pipelineBindPoint)
	{
		std::vector<VkDescriptorSet> sets;
		for(const auto s : setIds)
		{
			sets.push_back(getDescriptorSet(s)->mDescriptorSet);
		}

		vkCmdBindDescriptorSets(
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ?
			mComputeCommandBuffers[mImageIndex].mCommandBuffer :
			mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
			pipelineBindPoint,
			pipelineLayout,
			0,
			sets.size(),
			sets.data(),
			0,
			nullptr);
	}

	uint32_t VulkanRenderer::createSampler(const VulkanSamplerKey& key)
	{
		return mSamplerCache.findOrCreate(key, [this](const VulkanSamplerKey& key)
			{
				VkSamplerCreateInfo samplerCreateInfo = {};
				samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerCreateInfo.magFilter = key.mFilter;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.addressModeU = key.mAddressMode;
				samplerCreateInfo.addressModeV = key.mAddressMode;
				samplerCreateInfo.addressModeW = key.mAddressMode;
				//For border clamp only
				samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
				//normalized: 0-1 space, unnormalized: 0-imageSize
				samplerCreateInfo.unnormalizedCoordinates = key.mUnnormalizedCoordinates;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				//Level of details bias for mip level
				samplerCreateInfo.mipLodBias = 0.0f;
				samplerCreateInfo.minLod = 0.0f;
				samplerCreateInfo.maxLod = 0.0f;
				samplerCreateInfo.anisotropyEnable = VK_TRUE;
				//Anisotropy sample level
				samplerCreateInfo.maxAnisotropy = 16.0f;

				VkSampler result = VK_NULL_HANDLE;
				VK_CHECK(vkCreateSampler(mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &result));

				return result;
			});
	}

	VkSampler VulkanRenderer::getSampler(const uint32_t id)
	{
		return mSamplerCache.getByIndex(id);
	}

	VulkanTextureInfoPtr VulkanRenderer::createTextureInfo(
		const VkFormat format,
		const VkSamplerAddressMode addressMode,
		const VkImageTiling tiling,
		const VkImageUsageFlags usageFlags,
		const VkMemoryPropertyFlags memoryFlags,
		const VkImageLayout layout,
		const bool isExternal,
		Image& image)
	{
		return mTextureManager.createTextureInfo(
			format,
			addressMode,
			tiling,
			usageFlags,
			memoryFlags,
			layout,
			isExternal,
			image);
	}

	uint32_t VulkanRenderer::createTexture(const VulkanTextureInfoPtr& info)
	{
		return mTextureManager.createTexture(mainDevice, mTransferQueueFamilyId,
			mGraphicsQueueFamilyId, mGraphicsQueue, mGraphicsCommandPool, info);
	}

	VulkanTexturePtr VulkanRenderer::getTexture(const uint32_t id)
	{
		return mTextureManager.getTexture(id);
	}

	void VulkanRenderer::updateTextureImage(const VulkanTextureInfoPtr& info)
	{
		mTextureManager.updateTextureImage(mainDevice, mTransferQueueFamilyId, mGraphicsQueueFamilyId,
			mGraphicsQueue, mGraphicsCommandPool, info);
	}

	VulkanBuffer VulkanRenderer::createStagingBuffer(const void* data, size_t size)
	{
		auto result = mBufferManager.createStagingBuffer(
			mainDevice,
			mTransferQueue,
			mTransferCommandPool,
			data, size);
		return result;
	}

	const VulkanBuffer& VulkanRenderer::createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, void* data, size_t dataSize)
	{
		const auto& result = mBufferManager.createBuffer(
			mainDevice,
			mTransferQueue,
			mTransferCommandPool,
			usage,
			memoryFlags,
			data, dataSize);

		return result;
	}

	const VulkanBuffer& VulkanRenderer::createExternalBuffer(VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryFlags,
		VkExternalMemoryHandleTypeFlagsKHR extMemHandleType, VkDeviceSize size)
	{
		const auto& result = mBufferManager.createExternalBuffer(mainDevice, bufferUsage, memoryFlags, extMemHandleType, size);

		return result;
	}

	void VulkanRenderer::copyBuffer(VkBuffer src, VkBuffer dst, size_t dataSize, VkPipelineBindPoint pipelineBindPoint) const
	{
		fre::copyBuffer(mainDevice.logicalDevice,
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeQueue : mTransferQueue,
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeCommandPool : mTransferCommandPool,
			src, dst, dataSize);
	}

	MeshModel::Ptr& VulkanRenderer::addMeshModel(const MeshModel::MeshList& meshList)
	{
		mMeshModels.push_back(MeshModel::Ptr(new MeshModel(meshList)));
		return mMeshModels.back();
	}

	MeshModel::Ptr VulkanRenderer::getMeshModel(uint32_t modelId) const
	{
		MeshModel::Ptr result;

		if (modelId < mMeshModels.size())
		{
			result = mMeshModels[modelId];
		}

		return result;
	}

	MeshModel::Ptr VulkanRenderer::getMeshModel(uint32_t modelId)
	{
		MeshModel::Ptr result;

		if (modelId < mMeshModels.size())
		{
			result = mMeshModels[modelId];
		}

		return result;
	}

	void VulkanRenderer::update(const Camera& camera, const Light& light)
	{
		std::lock_guard<std::mutex> lock(gRenderMutex);
		if(needRedraw())
		{
			mHasComputeTasks = false;
			for(size_t j = 0; j < mMeshModels.size() && !mHasComputeTasks; j++)
			{
				auto& model = mMeshModels[j];

				for(size_t k = 0; k < model->getMeshCount() && !mHasComputeTasks; k++)
				{
					auto& mesh = model->getMesh(k);
					if(mesh->getVisible() && mesh->getComputeShaderId() != std::numeric_limits<uint32_t>::max())
					{
						mHasComputeTasks = true;
					}
				}
			}

			if(mHasComputeTasks)
			{
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				// Compute submission        
				std::vector<VkFence> fences = {mComputeFences[mCurrentFrame]};
				VK_CHECK(vkWaitForFences(mainDevice.logicalDevice, fences.size(),
					fences.data(), VK_TRUE, MAX(uint64_t)));

				VK_CHECK(vkResetFences(mainDevice.logicalDevice, 1, &mComputeFences[mCurrentFrame]));

				const auto commandBuffer = mComputeCommandBuffers[mImageIndex];
				VK_CHECK(vkResetCommandBuffer(commandBuffer.mCommandBuffer, 0));
				commandBuffer.begin();
				recordSceneCommands(camera, light, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
				commandBuffer.end();

				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer.mCommandBuffer;

				std::vector<VkSemaphore> waitSemaphores = {mRenderFinished[mCurrentFrame]};
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
				/*if(mFrameNumber > 0)
				{
					submitInfo.waitSemaphoreCount = waitSemaphores.size();
					submitInfo.pWaitSemaphores = waitSemaphores.data();
					submitInfo.pWaitDstStageMask = waitStages;	//Stages to check semaphores at
				}*/

				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &mComputeFinished[mCurrentFrame];

				VK_CHECK(vkQueueSubmit(mComputeQueue, 1, &submitInfo, mComputeFences[mCurrentFrame]));
			}
		}
	}

	void VulkanRenderer::draw(const Camera& camera, const Light& light)
	{
		if(needRedraw())
		{
			//Wait for given fence to signal (open) from last draw before continuing
			VK_CHECK(vkWaitForFences(mainDevice.logicalDevice, 1, &mDrawFences[mCurrentFrame],
				VK_TRUE, std::numeric_limits<uint32_t>::max()));

			//Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
			VkResult result = vkAcquireNextImageKHR(mainDevice.logicalDevice, mSwapChain.mSwapChain,
				std::numeric_limits<uint64_t>::max(), mImageAvailable[mCurrentFrame], VK_NULL_HANDLE, &mImageIndex);

			bool reshaped = false;
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				recreateSwapChain();
				reshaped = true;
			}
			else if (result != VK_SUCCESS/* && result != VK_SUBOPTIMAL_KHR*/)
			{
				throw std::runtime_error("failed to acquire swap chain image!");
			}
			else
			{
				// -- GET NEXT IMAGE--
				//Manually reset (close) fences
				VK_CHECK(vkResetFences(mainDevice.logicalDevice, 1, &mDrawFences[mCurrentFrame]));
				updateUniformBuffers(camera);
			
				VK_CHECK(vkResetCommandBuffer(mGraphicsCommandBuffers[mCurrentFrame].mCommandBuffer, 0));

				recordCommands(camera, light);

				// -- SUBMIT COMMAND BUFFER TO RENDER --
				//Queue submission info
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				std::vector<VkSemaphore> waitSemaphores;
				std::vector<VkPipelineStageFlags> waitStages;
				if(mExternalWaitSemaphore != VK_NULL_HANDLE)
				{
					waitSemaphores.push_back(mExternalWaitSemaphore);
				}
				waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				if(mHasComputeTasks)
				{
					waitSemaphores.push_back(mComputeFinished[mCurrentFrame]);
					waitStages.push_back(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
				}

				waitSemaphores.push_back(mImageAvailable[mCurrentFrame]);
				waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

				submitInfo.waitSemaphoreCount = waitSemaphores.size();		//Number of semaphores to wait on
				submitInfo.pWaitSemaphores = waitSemaphores.data();	//List of samephores to wait on
				submitInfo.pWaitDstStageMask = waitStages.data();	//Stages to check semaphores at
				submitInfo.commandBufferCount = 1;	//Number of command buffers to submit
				VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[mImageIndex].mCommandBuffer;
				submitInfo.pCommandBuffers = &commandBuffer;	//Command buffer to submit
				std::vector<VkSemaphore> signalSemaphores;
				if(mExternalSignalSemaphore != VK_NULL_HANDLE)
				{
					signalSemaphores.push_back(mExternalSignalSemaphore);
				}
				signalSemaphores.push_back(mRenderFinished[mCurrentFrame]);
				submitInfo.signalSemaphoreCount = signalSemaphores.size();	//Number of semaphores to signal
				submitInfo.pSignalSemaphores = signalSemaphores.data();	//Semaphore to signal when command buffer finishes

				VK_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mDrawFences[mCurrentFrame]));

				// -- PRESENT RENDERED IMAGE TO SCREEN --
				VkPresentInfoKHR presentInfo = {};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;	//Number of semaphores to wait on
				presentInfo.pWaitSemaphores = &mRenderFinished[mCurrentFrame];	//Semaphores to wait on
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = &mSwapChain.mSwapChain;	//Swapchains to present images to
				presentInfo.pImageIndices = &mImageIndex;	//Indices of images in swapchain to present

				result = vkQueuePresentKHR(mPresentationQueue, &presentInfo);

				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
				{
					mFramebufferResized = false;
					recreateSwapChain();
					reshaped = true;
				}
				else if (result != VK_SUCCESS)
				{
					throw std::runtime_error("failed to present swap chain image!");
				}

				VkResult vulkanResult = vkWaitForFences(mainDevice.logicalDevice, 1, &mDrawFences[mCurrentFrame],
					VK_TRUE, std::numeric_limits<uint32_t>::max());
				if(vulkanResult != VK_SUCCESS)
				{
					LOG_ERROR("Vulkan error {}", vulkanResult);
				}
				//Get next frame
				mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAME_DRAWS;
			}
			mFrameNumber++;
			if(!reshaped)
			{
				resetRedrawRequest();
			}
		}
	}

	void VulkanRenderer::preprocessUI()
	{
		if(!mUIFrameStarted)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			{
				for(auto& uiRenderCallback : mUIRenderCallbacks)
				{
					uiRenderCallback();
				}
			}
			mUIFrameStarted = true;
		}
	}

	void VulkanRenderer::drawUI()
	{
		if(mUIFrameStarted)
		{
			// Rendering
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, mGraphicsCommandBuffers[mImageIndex].mCommandBuffer);
			mUIFrameStarted = false;
		}
	}

	void VulkanRenderer::pushConstants(VkPushConstantRange pushConstants, const void* data, VkPipelineLayout pipelineLayout, VkPipelineBindPoint pipelineBindPoint)
	{
        vkCmdPushConstants(
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeCommandBuffers[mImageIndex].mCommandBuffer : mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
            pipelineLayout,
            pushConstants.stageFlags,
            pushConstants.offset,
            pushConstants.size,
            data);
	}

	void VulkanRenderer::cleanupSwapChainFrameBuffers()
	{
		//destroy frame buffers
		for(auto& fbo : mFrameBuffers)
		{
			fbo.destroy(mainDevice.logicalDevice);
		}
	}

	void VulkanRenderer::cleanupUniformDescriptorPool()
	{
		mUniformDescriptorPool.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupInputDescriptorPool()
	{
		mInputDescriptorPool.destroy(mainDevice.logicalDevice);
		mDepthDescriptorPool.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupUIDescriptorPool()
	{
		mUIDescriptorPool.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupInputDescriptorSetLayout()
	{
		mInputDescriptorSetLayout.destroy(mainDevice.logicalDevice);
		mDepthDescriptorSetLayout.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupUniformDescriptorSetLayout()
	{
		mUniformDescriptorSetLayout.destroy(mainDevice.logicalDevice);
	}

    void VulkanRenderer::cleanupSwapchainImagesSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, mImageAvailable[i], nullptr);
		}
    }

    void VulkanRenderer::cleanupRenderFinishedSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, mRenderFinished[i], nullptr);
		}
    }

    void VulkanRenderer::cleanupComputeFinishedSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, mComputeFinished[i], nullptr);
		}
    }
    
	void VulkanRenderer::cleanupDrawFences()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroyFence(mainDevice.logicalDevice, mDrawFences[i], nullptr);
		}
    }
    
	void VulkanRenderer::cleanupComputeFences()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroyFence(mainDevice.logicalDevice, mComputeFences[i], nullptr);
		}
    }

	void VulkanRenderer::cleanupTransferSynchronisation()
	{
		vkDestroySemaphore(mainDevice.logicalDevice, mTransferCompleteSemaphore, nullptr);
		vkDestroyFence(mainDevice.logicalDevice, mTransferFence, nullptr);
	}

	void VulkanRenderer::cleanupSemaphores()
	{
		for (auto semaphore : mSemaphores)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, semaphore, nullptr);
		}
	}

	void VulkanRenderer::cleanupUI()
	{
		if(ImGui::GetCurrentContext() != nullptr)
		{
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			ImPlot::DestroyContext();
		}
	}

	AccelerationStructure& VulkanRenderer::createBLAS(VulkanBuffer& vbo, VulkanBuffer& ibo, VulkanBuffer& transform)
	{
		VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
		VkDeviceOrHostAddressConstKHR index_data_device_address{};
		VkDeviceOrHostAddressConstKHR transform_matrix_device_address{};

		vertex_data_device_address.deviceAddress = vbo.mDeviceAddress;
		index_data_device_address.deviceAddress = ibo.mDeviceAddress;
		transform_matrix_device_address.deviceAddress = transform.mDeviceAddress;

		// The bottom level acceleration structure contains one set of triangles as the input geometry
		VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
		acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_structure_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		acceleration_structure_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_structure_geometry.geometry.triangles.vertexData = vertex_data_device_address;
		acceleration_structure_geometry.geometry.triangles.maxVertex = 3;
		acceleration_structure_geometry.geometry.triangles.vertexStride = sizeof(Vertex);
		acceleration_structure_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		acceleration_structure_geometry.geometry.triangles.indexData = index_data_device_address;
		acceleration_structure_geometry.geometry.triangles.transformData = transform_matrix_device_address;

		AccelerationStructure& result = buildAccelerationStructure(acceleration_structure_geometry, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

		return result;
	}

	AccelerationStructure& VulkanRenderer::createTLAS(const uint64_t refBlasAddress, const VkTransformMatrixKHR& transform)
	{
		VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
		acceleration_structure_instance.transform = transform;
		acceleration_structure_instance.instanceCustomIndex = 0;
		acceleration_structure_instance.mask = 0xFF;
		acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
		acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		acceleration_structure_instance.accelerationStructureReference = refBlasAddress;

		const VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VulkanBuffer instancesBuffer = createBuffer(buffer_usage_flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &acceleration_structure_instance, sizeof(VkAccelerationStructureInstanceKHR));
		
		VkDeviceOrHostAddressConstKHR instance_data_device_address{};
		instance_data_device_address.deviceAddress = instancesBuffer.mDeviceAddress;

		// The top level acceleration structure contains (bottom level) instance as the input geometry
		VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
		acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
		acceleration_structure_geometry.geometry.instances.data = instance_data_device_address;

		AccelerationStructure& result = buildAccelerationStructure(acceleration_structure_geometry, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

		return result;
	}

	AccelerationStructure& VulkanRenderer::buildAccelerationStructure(VkAccelerationStructureGeometryKHR& acceleration_structure_geometry, const VkAccelerationStructureTypeKHR asType)
	{
		// Get the size requirements for buffers involved in the acceleration structure build process
		VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
		acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_structure_build_geometry_info.type = asType;
		acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_structure_build_geometry_info.geometryCount = 1;
		acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

		const uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
		acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			mainDevice.logicalDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&acceleration_structure_build_geometry_info,
			&primitive_count,
			&acceleration_structure_build_sizes_info);

		// Create a buffer to hold the acceleration structure
		mAccelerationStructures.emplace_back(AccelerationStructure());
		AccelerationStructure& as = mAccelerationStructures.back();
		as.mBuffer = createBuffer(
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&as.mDeviceAddress,
			acceleration_structure_build_sizes_info.accelerationStructureSize);

		// Create the acceleration structure
		VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
		acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_structure_create_info.buffer = as.mBuffer.mBuffer;
		acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
		acceleration_structure_create_info.type = asType;
		vkCreateAccelerationStructureKHR(mainDevice.logicalDevice, &acceleration_structure_create_info, nullptr, &as.mHandle);

		// The actual build process starts here

		// Create a scratch buffer as a temporary storage for the acceleration structure build
		VulkanBuffer scratch_buffer = createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			nullptr,
			acceleration_structure_build_sizes_info.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type = asType;
		acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		acceleration_build_geometry_info.dstAccelerationStructure = as.mHandle;
		acceleration_build_geometry_info.geometryCount = 1;
		acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
		acceleration_build_geometry_info.scratchData.deviceAddress = scratch_buffer.mDeviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
		acceleration_structure_build_range_info.primitiveCount = 1;
		acceleration_structure_build_range_info.primitiveOffset = 0;
		acceleration_structure_build_range_info.firstVertex = 0;
		acceleration_structure_build_range_info.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		VulkanCommandBuffer command_buffer;
		command_buffer.allocate(mGraphicsCommandPool, mainDevice.logicalDevice);
		command_buffer.begin();
		vkCmdBuildAccelerationStructuresKHR(
			command_buffer.mCommandBuffer,
			1,
			&acceleration_build_geometry_info,
			acceleration_build_structure_range_infos.data());

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = 0;
		VkFence fence;
		VK_CHECK(vkCreateFence(mainDevice.logicalDevice, &fence_info, nullptr, &fence));
		std::vector<VkSemaphore> semaphores;
		command_buffer.flush(mainDevice.logicalDevice, mGraphicsQueue, fence, semaphores);

		vkDestroyFence(mainDevice.logicalDevice, fence, nullptr);

		mBufferManager.destroyBuffer(mainDevice.logicalDevice, scratch_buffer);

		// Get the bottom acceleration structure's handle, which will be used later
		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = as.mHandle;
		as.mDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(mainDevice.logicalDevice, &acceleration_device_address_info);

		return as;
	}

	void VulkanRenderer::destroyAccelerationStructure(AccelerationStructure& accelerationStructure)
	{
		if (accelerationStructure.mHandle)
		{
			vkDestroyAccelerationStructureKHR(mainDevice.logicalDevice, accelerationStructure.mHandle, nullptr);
		}
	}

	void VulkanRenderer::cleanupRayTracing()
	{
		for(auto& as : mAccelerationStructures)
		{
			destroyAccelerationStructure(as);
		}
	}

    void VulkanRenderer::setFramebufferResized(bool resized)
    {
		mFramebufferResized = resized;
    }

	void VulkanRenderer::setClearColor(const vec4& clearColor)
	{
		mClearColor = clearColor;
	}

	void VulkanRenderer::setDefaultShaderFileName(const std::string& shaderFileName)
	{
		mDefaultShaderFileName = shaderFileName;
	}

	void VulkanRenderer::setDefaultShininess(const float shininess)
	{
		mDefaultShininess = shininess;
	}

	void VulkanRenderer::fillLightingPushConstant(const Mesh::Ptr& mesh, const mat4& modelMatrix, const Camera& camera, const Light& light, Lighting& lighting)
	{
		mat3 normalMatrix(modelMatrix);
		normalMatrix = transpose(inverse(normalMatrix));
		lighting.normalMatrix = mat4(normalMatrix);
		lighting.cameraEye = vec4(-camera.getEye(), 0.0);
		const auto& material = mMaterials[mesh->getMaterialId()];
		lighting.lightPos = vec4(light.mPosition, material.mShininess);
		lighting.lightDiffuseColor = vec4(light.mDiffuseColor, 1.0f);
		lighting.lightSpecularColor = vec4(light.mSpecularColor, 1.0f);
	}

	void VulkanRenderer::transitionDepthLayout(VkImageLayout from, VkImageLayout to, VkPipelineBindPoint pipelineBindPoint)
	{
		transitionImageLayout(mainDevice.logicalDevice,
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeQueue : mGraphicsQueue,
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeCommandPool : mGraphicsCommandPool,
			mFrameBuffers[mImageIndex].mDepthAttachment.mImage,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			from, to);
	}

	Material& VulkanRenderer::getMaterial(uint32_t id)
	{
		assert(id < mMaterials.size());

		return mMaterials[id];
	}

	void VulkanRenderer::requestExtensions()
	{
		{
			// API interoperation
			addDeviceExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
			addDeviceExtension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
			addDeviceExtension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
#ifdef _WIN64
			addDeviceExtension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
			addDeviceExtension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#else
			addDeviceExtension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
			addDeviceExtension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#endif /* _WIN64 */
			addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		{
			//Instance extensions
			uint32_t glfwExtensionsCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
			for (size_t i = 0; i < glfwExtensionsCount; i++)
			{
				addInstanceExtension(glfwExtensions[i]);
			}

			//Interoperation
			addInstanceExtension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
			addInstanceExtension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);

			//To check device features support
			addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}
	}

	void VulkanRenderer::requestDeviceFeatures()
	{
		mDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		mDeviceFeatures.features.fillModeNonSolid = VK_TRUE;
		mDeviceFeatures.features.wideLines = VK_TRUE;
		mDeviceFeatures.features.samplerAnisotropy = VK_TRUE;
		mLastDeviceFeatures = &mDeviceFeatures.pNext;
	}

    void VulkanRenderer::createInstance()
    {
		LOG_INFO("Create Vulkan instance");

		volkInitialize();

		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			enableValidationLayers = false;
			LOG_ERROR("Validation layers requested, but not available!");
		}

		//Information about application itself
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan App";
		// Custom version of the application
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = project_name.data();
		appInfo.engineVersion = VK_MAKE_VERSION(project_version_major, project_version_minor, project_version_patch);	//Custom engine version
		//Vulkan version
		appInfo.apiVersion = VK_API_VERSION_1_1;

		//Creation information for VkInstance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//Check instance extensions supported
		if (!checkInstanceExtensionsSupport(&mRequestedInstanceExtensions))
		{
			throw std::runtime_error("VkInstance does not support required extensions!");
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(mRequestedInstanceExtensions.size());
		createInfo.ppEnabledExtensionNames = mRequestedInstanceExtensions.data();
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		//Create instance
		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mInstance));

		volkLoadInstance(mInstance);

		LOG_INFO("Vulkan instance created");
	}

	void VulkanRenderer::createLogicalDevice()
	{
		LOG_INFO("Create logical device");

		//Vector for queue creation information, and set for family indices
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		
		int8_t graphicsQueueId = -1;
		int8_t presentationQueueId = -1;
		int8_t transferQueueId = -1;
		int8_t computeQueueId = -1;
		std::vector<float> queuePriorities;
		//Queue the logical device needs to create and info to do so
		for(const auto& queueFamily : mQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			//The index of the family to create queue from
			queueCreateInfo.queueFamilyIndex = queueFamily.mId;
			//Number of queues to create
			queueCreateInfo.queueCount = 0;
			if(queueFamily.mHasGraphicsSupport &&
				mGraphicsQueueFamilyId == -1)
			{
				if(queueCreateInfo.queueCount < queueFamily.mQueueCount)
				{
					queueCreateInfo.queueCount++;
					queuePriorities.push_back(1.0f);
				}
				graphicsQueueId = queueCreateInfo.queueCount - 1;
				mGraphicsQueueFamilyId = queueFamily.mId;
			}
			if(queueFamily.mHasPresentationSupport &&
				mPresentationQueueFamilyId == -1)
			{
				if(queueCreateInfo.queueCount < queueFamily.mQueueCount)
				{
					queueCreateInfo.queueCount++;
					queuePriorities.push_back(1.0f);
				}
				presentationQueueId = queueCreateInfo.queueCount - 1;
				mPresentationQueueFamilyId = queueFamily.mId;
			}
			if(queueFamily.mHasTransferSupport &&
				mTransferQueueFamilyId == -1)
			{
				if(queueCreateInfo.queueCount < queueFamily.mQueueCount)
				{
					queueCreateInfo.queueCount++;
					queuePriorities.push_back(1.0f);
				}
				transferQueueId = queueCreateInfo.queueCount - 1;
				mTransferQueueFamilyId = queueFamily.mId;
			}
			if(queueFamily.mHasComputeSupport &&
				mComputeQueueFamilyId == -1)
			{
				if(queueCreateInfo.queueCount < queueFamily.mQueueCount)
				{
					queueCreateInfo.queueCount++;
					queuePriorities.push_back(1.0f);
				}
				computeQueueId = queueCreateInfo.queueCount - 1;
				mComputeQueueFamilyId = queueFamily.mId;
			}
			//Vulkan needs to know how to handle multiple queues, so decide priority (1 - highest priority)
			queueCreateInfo.pQueuePriorities = queuePriorities.data();

			queueCreateInfos.push_back(queueCreateInfo);

			if(mGraphicsQueueFamilyId != -1 && mPresentationQueueFamilyId != -1 && mTransferQueueFamilyId != -1)
			{
				break;
			}
		}

		//Information to create logical device
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		//Number of queue create infos
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequestedDeviceExtensions.size());	//Number of enabled logical device extensions
		deviceCreateInfo.ppEnabledExtensionNames = mRequestedDeviceExtensions.data();

		deviceCreateInfo.pEnabledFeatures = nullptr;
		//Vulkan V1.1 style to enable device features
		deviceCreateInfo.pNext = &mDeviceFeatures;

		//Create logical device for the given physical device
		VK_CHECK(vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice));

		volkLoadDevice(mainDevice.logicalDevice);

		//Queues are created at the same time as the device
		//So we want hande to queues
		//From given logical device, of given Queue Family, of given queue index (0 since only one queue), place reference in given VkQueue
		vkGetDeviceQueue(mainDevice.logicalDevice, mGraphicsQueueFamilyId, graphicsQueueId, &mGraphicsQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, mPresentationQueueFamilyId, presentationQueueId, &mPresentationQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, mTransferQueueFamilyId, transferQueueId, &mTransferQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, mComputeQueueFamilyId, computeQueueId, &mComputeQueue);
		
		VkPhysicalDeviceIDProperties vkPhysicalDeviceIDProperties = {};
		vkPhysicalDeviceIDProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
		vkPhysicalDeviceIDProperties.pNext = NULL;
		VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2 = {};
		vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceIDProperties;

		PFN_vkGetPhysicalDeviceProperties2 fpGetPhysicalDeviceProperties2;
		fpGetPhysicalDeviceProperties2 =
			(PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(
				mInstance, "vkGetPhysicalDeviceProperties2");
		if(fpGetPhysicalDeviceProperties2 == NULL)
		{
			throw std::runtime_error(
				"Vulkan: Proc address for \"vkGetPhysicalDeviceProperties2KHR\" not "
				"found.\n");
		}

		fpGetPhysicalDeviceProperties2(mainDevice.physicalDevice,
			&vkPhysicalDeviceProperties2);

		memcpy(mDeviceUUID, vkPhysicalDeviceIDProperties.deviceUUID, VK_UUID_SIZE);
		LOG_TRACE("Physical device UUID: {} {} {} {} {} {} {} {}",
			mDeviceUUID[0], mDeviceUUID[1], mDeviceUUID[2], mDeviceUUID[3],
			mDeviceUUID[4], mDeviceUUID[5], mDeviceUUID[6], mDeviceUUID[7]);

		LOG_INFO("Logical device created");
	}

	void VulkanRenderer::createSwapChainFrameBuffers()
	{
		LOG_INFO("Create swapchain framebuffers");

		mFrameBuffers.resize(mSwapChain.mSwapChainImages.size());
		
		for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
		{
			VulkanAttachment colorAttachment;
			VulkanAttachment depthAttachment;
			
			colorAttachment.create(mainDevice, COLOR_ATTACHMENT, mSwapChain.mSwapChainExtent);
			depthAttachment.create(mainDevice, DEPTH_ATTACHMENT, mSwapChain.mSwapChainExtent);

			std::vector<VkImageView> attachmentsViews =
			{
				mSwapChain.mSwapChainImages[i].imageView,
				colorAttachment.mImageView,
				depthAttachment.mImageView
			};

			mFrameBuffers[i].create(mainDevice,
				attachmentsViews,
				mSwapChain.mSwapChainExtent,
				mRenderPass.mRenderPass);
			mFrameBuffers[i].addColorAttachment(colorAttachment);
			mFrameBuffers[i].setDepthAttachment(depthAttachment);
		}

		LOG_INFO("Swapchain framebuffers created");
	}

	void VulkanRenderer::createSurface()
	{
		LOG_INFO("Create window surface");

		//Create surface (creates a surface create info struct, runs the create surface function, returns result)
		VkResult result = glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error(formatString("Failed to create a surface! Error code: %i", result));
		}

		LOG_INFO("Window surface created");
	}

	void VulkanRenderer::createInputDescriptorPool()
	{
		LOG_INFO("Create input descriptor pool");

		//pool for color and depth attachments
		mInputDescriptorPool.create(
			mainDevice.logicalDevice,
			0,
			MAX_FRAME_DRAWS,
			{
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_FRAME_DRAWS}
			});
		mDepthDescriptorPool.create(
			mainDevice.logicalDevice,
			0,
			MAX_FRAME_DRAWS,
			{
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAME_DRAWS}
			});

		LOG_INFO("Input descriptor pool created");
	}

	void VulkanRenderer::createUIDescriptorPool()
	{
		//pool for color and depth attachments
		mUIDescriptorPool.create(
			mainDevice.logicalDevice,
			0,
			MAX_OBJECTS,
			{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS}});
	}

	void VulkanRenderer::createInputDescriptorSetLayout()
	{
		LOG_INFO("Create input descriptor set layout");

		mInputDescriptorSetLayout.create(
			mainDevice.logicalDevice,
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT},
			{VK_SHADER_STAGE_FRAGMENT_BIT });
		mDepthDescriptorSetLayout.create(
			mainDevice.logicalDevice,
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
			{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT});

		LOG_INFO("Input descriptor set layout created");
	}

	void VulkanRenderer::createPipelines()
	{
		for(auto& shader : mShaders)
		{
			LOG_TRACE("Create pipeline for shader: {}", shader.mName);

			ShaderMetaDatum& shaderMetaDatum = mShaderMetaDatum[shader.mId];
			for(const auto& shaderMetaData : shaderMetaDatum)
			{
				if(shader.mComputeShader.mShaderModule != VK_NULL_HANDLE)
				{
					shader.mComputePipelineIds.push_back(static_cast<uint32_t>(mPipelines.size()));
					mPipelines.push_back(VulkanPipeline());
					auto& pipeline = mPipelines.back();
					pipeline.createComputePipeline(
						mainDevice.logicalDevice,
						shader.mComputeShader,
						shaderMetaData.mDescriptorSetLayouts,
						shaderMetaData.mPushConstantRanges);
				}

				if(
					shader.mVertexShader.mShaderModule != VK_NULL_HANDLE ||
					shader.mFragmentShader.mShaderModule != VK_NULL_HANDLE)
				{
					shader.mGraphicsPipelineIds.push_back(static_cast<uint32_t>(mPipelines.size()));
					mPipelines.push_back(VulkanPipeline());
					auto& pipeline = mPipelines.back();
					pipeline.createGeometryPipeline(
						mainDevice.logicalDevice,
						{&shader.mVertexShader, &shader.mFragmentShader},
						shaderMetaData.mTopology,
						shaderMetaData.mVertexSize,
						shaderMetaData.mVertexAttributes,
						shaderMetaData.mDepthTestEnabled ? VK_TRUE : VK_FALSE,
						mRenderPass.mRenderPass,
						shaderMetaData.mSubPassIndex,
						shaderMetaData.mDescriptorSetLayouts,
						shaderMetaData.mPushConstantRanges,
						shaderMetaData.mAttachmentsCount,
						shaderMetaData.mLineWidth,
						shaderMetaData.mCullMode
					);
				}

				if(
					shader.mRayGenShader.mShaderModule != VK_NULL_HANDLE &&
					shader.mRayMissShader.mShaderModule != VK_NULL_HANDLE &&
					shader.mRayClosestHitShader.mShaderModule != VK_NULL_HANDLE)
				{
					shader.mRTPipelineIds.push_back(static_cast<uint32_t>(mPipelines.size()));
					mPipelines.push_back(VulkanPipeline());
					auto& pipeline = mPipelines.back();
					pipeline.createRTPipeline(
						mainDevice.logicalDevice,
						{&shader.mRayGenShader, &shader.mRayMissShader,&shader.mRayClosestHitShader},
						shaderMetaData.mDescriptorSetLayouts,
						shaderMetaData.mPushConstantRanges);
					pipeline.createShaderBindingTables(mainDevice, mTransferQueue, mTransferCommandPool,
						mRayTracingPipelineProperties, mBufferManager);
				}

			}
			
			//Destroy shader modules, no longer needed after Pipeline created
			shader.destroy(mainDevice.logicalDevice);
		}

		//2 sub passes each using 1 pipeline
        mSubPassesCount = 2;
	}

	void VulkanRenderer::cleanupPipelines(VkDevice logicalDevice)
	{
		for(auto& pipeline : mPipelines)
		{
			pipeline.destroy(logicalDevice);
		}

		for(auto& shader : mShaders)
		{
			//In general shader modules are destroyed after corresponding pipelines created
			//But if pipeline was not created we should destroy shader modules anyway at exit
			shader.mVertexShader.destroy(logicalDevice);
			shader.mFragmentShader.destroy(logicalDevice);
			shader.mComputeShader.destroy(logicalDevice);
		}
	}

	void VulkanRenderer::createCommandPools()
	{
		LOG_INFO("Create command pools");

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = mGraphicsQueueFamilyId;	//Queue family type that buffers trom this command pool will use

		//Create a Graphics Queue Family Command Pool
		VK_CHECK(vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &mGraphicsCommandPool));

		poolInfo.queueFamilyIndex = mTransferQueueFamilyId;

		VK_CHECK(vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &mTransferCommandPool));
		
		poolInfo.queueFamilyIndex = mComputeQueueFamilyId;

		VK_CHECK(vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &mComputeCommandPool));

		LOG_INFO("Command pools created");
	}

	void VulkanRenderer::createCommandBuffers()
	{
		LOG_INFO("Create command buffers");

		mGraphicsCommandBuffers.resize(mFrameBuffers.size());
		mTransferCommandBuffers.resize(mFrameBuffers.size());
		mComputeCommandBuffers.resize(mFrameBuffers.size());
		for(int i = 0; i < mFrameBuffers.size(); i++)
		{
			mGraphicsCommandBuffers[i].allocate(mGraphicsCommandPool, mainDevice.logicalDevice);
			mTransferCommandBuffers[i].allocate(mTransferCommandPool, mainDevice.logicalDevice);
			mComputeCommandBuffers[i].allocate(mComputeCommandPool, mainDevice.logicalDevice);
		}

		LOG_INFO("Command buffers created");
	}

	void VulkanRenderer::allocateInputDescriptorSets()
	{
		LOG_INFO("Allocate input descriptor sets");

		//Resize descriptor set array for each swap chain image
		mInputDescriptorSets.resize(MAX_FRAME_DRAWS);
		mDepthDescriptorSets.resize(MAX_FRAME_DRAWS);
		for(uint32_t i = 0; i < mInputDescriptorSets.size(); i++)
		{
			mInputDescriptorSets[i].allocate(mainDevice.logicalDevice,
				mInputDescriptorPool.mDescriptorPool,
				mInputDescriptorSetLayout.mDescriptorSetLayout);
			mInputDescriptorSets[i].update(
				mainDevice.logicalDevice,
				{
					std::make_shared<DescriptorImage>(
						VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						mFrameBuffers[i].mColorAttachments[0].mImageView,
						//We don't need a sampler, because we will read it in other way in shader
						VK_NULL_HANDLE)
				}
			);

			mDepthDescriptorSets[i].allocate(mainDevice.logicalDevice,
				mDepthDescriptorPool.mDescriptorPool,
				mDepthDescriptorSetLayout.mDescriptorSetLayout);
			auto samplerId = createSampler({ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, VK_FALSE });
			mDepthDescriptorSets[i].update(
				mainDevice.logicalDevice,
				{
					std::make_shared<DescriptorImage>(
						VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
						mFrameBuffers[i].mDepthAttachment.mImageView,
						getSampler(samplerId))
				});
		}

		LOG_INFO("Input descriptor sets allocated");
	}

	static PFN_vkVoidFunction VKAPI_PTR imguiVulkanFunctionLoader(const char* name, void* user_data)
	{
		return vkGetInstanceProcAddr((VkInstance)user_data, name);
	}

	void VulkanRenderer::createUI()
	{
		createUIDescriptorPool();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(mWindow, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = mInstance;
		init_info.PhysicalDevice = mainDevice.physicalDevice;
		init_info.Device = mainDevice.logicalDevice;
		init_info.QueueFamily = mGraphicsQueueFamilyId;
		init_info.Queue = mGraphicsQueue;
		//init_info.PipelineCache = g_PipelineCache;
		init_info.DescriptorPool = mUIDescriptorPool.mDescriptorPool;
		init_info.RenderPass = mRenderPass.mRenderPass;
		init_info.Subpass = 1;
		init_info.MinImageCount = MAX_FRAME_DRAWS;
		init_info.ImageCount = MAX_FRAME_DRAWS;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = nullptr;

		bool result = ImGui_ImplVulkan_LoadFunctions(imguiVulkanFunctionLoader, mInstance);
		if (!result)
		{
			LOG_ERROR("ImGui_ImplVulkan_LoadFunctions(): failed");
		}

		ImGui_ImplVulkan_Init(&init_info);
	}

	void VulkanRenderer::updateUniformBuffers(const Camera& camera)
	{
		//Copy VP data
		/*void* data;
		UboViewProjection vp;
		vp.mView = camera.mView;
		vp.mProjection = camera.mProjection;
		VK_CHECK(vkMapMemory(mainDevice.logicalDevice, mVPUniformBufferMemory[mImageIndex], 0, sizeof(UboViewProjection), 0, &data));
		memcpy(data, &vp, sizeof(UboViewProjection));
		vkUnmapMemory(mainDevice.logicalDevice, mVPUniformBufferMemory[mImageIndex]);*/

		//Copy ModelMatrix data
		/*for (size_t i = 0; i < meshList.size(); i++)
		{
			ModelMatrix* thisModel = (ModelMatrix*)((uint64_t)modetTransferSpace + i * modelUniformAlignment);
			*thisModel = meshList[i].getModelMatrix();
		}
		//Map the list of model data
		vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[mImageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
		memcpy(data, modetTransferSpace, modelUniformAlignment * meshList.size());
		vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[mImageIndex]);*/
	}

	void VulkanRenderer::recordMeshCommands(
		const MeshModel::Ptr& model, const Mesh::Ptr& mesh, const Camera& camera,
		const Light& light, VkPipelineBindPoint pipelineBindPoint, uint32_t subPass, uint32_t instanceId)
	{
		const auto& material = mMaterials[mesh->getMaterialId()];
		const auto computeShaderId = mesh->getComputeShaderId();
		if((pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE || mesh->getComputeShaderId() != std::numeric_limits<uint32_t>::max())
			&& model->isVisible())
		{
			const auto& shader = pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mShaders[computeShaderId] : mShaders[material.mShaderId];

			const auto* vertexBuffer = getVertexBuffer(mesh->getId());
			bool needToProcess =
				mesh->getGeneratedVerticesCount() > 0 ||
				vertexBuffer != nullptr ||
				shader.mComputeShader.mShaderStage != 0;
			if(needToProcess)
			{
				const auto& pipelineIds = pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? shader.mComputePipelineIds :
					(shader.mRayGenShader.mShaderStage != 0 ? shader.mRTPipelineIds : shader.mGraphicsPipelineIds);
				const auto& shaderMetaDatum = mShaderMetaDatum[shader.mId];
				for(uint32_t i = 0; i < pipelineIds.size(); i++)
				{
					const auto& pipeline = mPipelines[pipelineIds[i]];
					const auto& shaderMetaData = shaderMetaDatum[i];

					if(shaderMetaData.mSubPassIndex == subPass && pipeline.mBindPoint == pipelineBindPoint) 
					{
						if(mesh->getBeforeRecordCallback() != nullptr)
						{
							mesh->getBeforeRecordCallback()(this, subPass, pipelineBindPoint);
						}
						
						bindPipeline(pipeline);

						const mat4& modelMatrix = model->getModelMatrix();

						//"Push" constants to given shader stage directly (no buffer)
					
						if(shaderMetaData.mPushConstantsCallback != nullptr)
						{
							shaderMetaData.mPushConstantsCallback(mesh, modelMatrix, camera, light, pipeline.mPipelineLayout, instanceId);
						}

                        if(mesh->mPushConstantsCallback != nullptr)
                        {
                            mesh->mPushConstantsCallback(mesh, modelMatrix, camera, light, pipeline.mPipelineLayout, instanceId);
                        }
			
						if(vertexBuffer != nullptr && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
						{
							VkBuffer vertexBuffers[] = { vertexBuffer->mBuffer };	//Buffers to bind
							VkDeviceSize offsets[] = { 0 };		//Offsets into buffers being bound
							bindVertexBuffers(vertexBuffers, 1, offsets, pipelineBindPoint);
						}

						const auto* indexBuffer = getIndexBuffer(mesh->getId());
						if(indexBuffer != nullptr && pipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
						{
							bindIndexBuffer(indexBuffer->mBuffer, pipelineBindPoint);
						}

                        if(mesh->getDescriptorSets().empty())
                        {
							for(const auto& dslId : shader.mDSLs)
							{
								std::vector<uint32_t> descriptorSetIds;
								const VulkanDescriptorSetKey key = { shader.mId, mSharedDescriptorPoolId, dslId, mesh->getId() };
								auto setId = createDescriptorSet(key);
                                descriptorSetIds.push_back(setId);
							}
						}

                        auto descriptorSets = mesh->getDescriptorSets();
						auto shaderInputs = mesh->getDescriptors();
						if(descriptorSets.size() == shaderInputs.size())
						{
							for(int i = 0; i < descriptorSets.size(); i++)
							{
								const auto& dsId = descriptorSets[i];
								auto& descriptorSet = getDescriptorSet(dsId);
								descriptorSet->update(mainDevice.logicalDevice, shaderInputs[i]);
							}
						}

                        //Bind descriptor sets
						bindDescriptorSets(mesh->getDescriptorSets(), pipeline.mPipelineLayout, pipelineBindPoint);

						if(shaderMetaData.mBindDescriptorSetsCallback != nullptr)
						{
							shaderMetaData.mBindDescriptorSetsCallback(mesh, material, pipeline.mPipelineLayout, instanceId);
						}

						auto commandBuffer = pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ?
							mComputeCommandBuffers[mImageIndex].mCommandBuffer :
							mGraphicsCommandBuffers[mImageIndex].mCommandBuffer;

						switch(pipeline.mBindPoint)
						{
						case VK_PIPELINE_BIND_POINT_COMPUTE:
							{
								const auto& computeSpace = mesh->getComputeSpace();
								vkCmdDispatch(commandBuffer, computeSpace.x, computeSpace.y, computeSpace.z);
							}
							break;
						case VK_PIPELINE_BIND_POINT_GRAPHICS:
							{
								if(shaderMetaData.mLineWidth > 0.0f)
								{
									vkCmdSetLineWidth(commandBuffer, shaderMetaData.mLineWidth);
								}
								if(mesh->getGeneratedVerticesCount() > 0)
								{
									vkCmdDraw(commandBuffer, mesh->getGeneratedVerticesCount(), 1, 0, 0);
								}
								else if(indexBuffer != nullptr)
								{
									vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->getIndexCount()), 1, 0, 0, 0);
								}
								else
								{
									vkCmdDraw(commandBuffer, mesh->getVertexCount(), 1, 0, 0);
								}
							}
							break;
						case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
							{
								const uint32_t handle_size_aligned = alignedSize(mRayTracingPipelineProperties.shaderGroupHandleSize, mRayTracingPipelineProperties.shaderGroupHandleAlignment);
								VkStridedDeviceAddressRegionKHR raygen_shader_sbt_entry{};
								raygen_shader_sbt_entry.deviceAddress = pipeline.mRaygenShaderBindingTable.mDeviceAddress;
								raygen_shader_sbt_entry.stride = handle_size_aligned;
								raygen_shader_sbt_entry.size = handle_size_aligned;

								VkStridedDeviceAddressRegionKHR miss_shader_sbt_entry{};
								miss_shader_sbt_entry.deviceAddress = pipeline.mMissShaderBindingTable.mDeviceAddress;
								miss_shader_sbt_entry.stride = handle_size_aligned;
								miss_shader_sbt_entry.size = handle_size_aligned;

								VkStridedDeviceAddressRegionKHR hit_shader_sbt_entry{};
								hit_shader_sbt_entry.deviceAddress = pipeline.mHhitShaderBindingTable.mDeviceAddress;
								hit_shader_sbt_entry.stride = handle_size_aligned;
								hit_shader_sbt_entry.size = handle_size_aligned;

								VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

								vkCmdTraceRaysKHR(
									commandBuffer,
									&raygen_shader_sbt_entry,
									&miss_shader_sbt_entry,
									&hit_shader_sbt_entry,
									&callable_shader_sbt_entry,
									mSwapChain.mSwapChainExtent.width,
									mSwapChain.mSwapChainExtent.height,
									1);
							}
							break;
						}
						if(mesh->getAfterRecordCallback() != nullptr)
						{
							mesh->getAfterRecordCallback()(this, subPass, pipelineBindPoint);
						}
					}
				}
			}
		}
	}

	void VulkanRenderer::recordSceneCommands(const Camera& camera, const Light& light, VkPipelineBindPoint pipelineBindPoint, uint32_t subPass)
	{
		for (size_t j = 0; j < mMeshModels.size(); j++)
		{
			auto& model = mMeshModels[j];

			for (size_t k = 0; k < model->getMeshCount(); k++)
			{
				const auto& mesh = model->getMesh(k);
				if(mesh->getBeforeVisitCallback())
				{
					mesh->getBeforeVisitCallback()(this, subPass, pipelineBindPoint);
				}
				if(mesh->getVisible())
				{
					auto count = mesh->getInstanceCount();
					for(uint32_t i = 0; i < count; i++)
					{
						recordMeshCommands(model, mesh, camera, light, pipelineBindPoint, subPass, i);
					}
				}
				if(mesh->getAfterVisitCallback())
				{
					mesh->getAfterVisitCallback()(this, subPass, pipelineBindPoint);
				}
			}
		}
	}

	void VulkanRenderer::renderFullscreenTriangle(VkPipelineLayout pipelineLayout)
	{
		VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[mImageIndex].mCommandBuffer;
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

    BoundingBox2D VulkanRenderer::getViewport() const
    {
        return BoundingBox2D(
			vec2(0.0f),
			vec2(mSwapChain.mSwapChainExtent.width, mSwapChain.mSwapChainExtent.height));
    }

	void VulkanRenderer::renderSubPass(uint32_t subPassIndex, const Camera& camera,
		const Light& light)
	{
		auto maxViewSize = getViewport();
		setViewport(maxViewSize);
        setScissor(maxViewSize);
        switch(subPassIndex)
        {
			case 0:
				{
					recordSceneCommands(camera, light, VK_PIPELINE_BIND_POINT_GRAPHICS, subPassIndex);
				}
				break;
			case 1:
			{
				const auto& fogShader = mShaders[mFogShaderId];
				const auto& shaderMetaDatum = mShaderMetaDatum[mFogShaderId];
				for(uint32_t i = 0; i < fogShader.mGraphicsPipelineIds.size(); i++)
				{
					const auto& pipelineId = fogShader.mGraphicsPipelineIds[i];
					const auto& fogPipeline = mPipelines[pipelineId];
					const auto& shaderMetaData = shaderMetaDatum[i];
					bindPipeline(fogPipeline);
					if (shaderMetaData.mPushConstantsCallback != nullptr)
					{
						shaderMetaData.mPushConstantsCallback(nullptr, mat4(1.0f), camera, light, fogPipeline.mPipelineLayout, 0);
					}

					bindDescriptorSets(
						{
							mInputDescriptorSets[mImageIndex].mDescriptorSet,
							mDepthDescriptorSets[mImageIndex].mDescriptorSet
						},
						fogPipeline.mPipelineLayout,
						VK_PIPELINE_BIND_POINT_GRAPHICS);
					renderFullscreenTriangle(fogPipeline.mPipelineLayout);
				}
			
				recordSceneCommands(camera, light, VK_PIPELINE_BIND_POINT_GRAPHICS, subPassIndex);
				drawUI();
            }
            break;
        }
	}

	void VulkanRenderer::loadShaderStage(
		ShaderInputParser& parser,
		VulkanShader& shader,
		const std::string& shaderFileName,
		VkShaderStageFlagBits stage,
		std::vector<VulkanDescriptorSetLayoutInfo>& layouts)
	{
		std::string stageStr;
		switch(stage)
		{
			case VK_SHADER_STAGE_VERTEX_BIT: stageStr = "vert"; break;
			case VK_SHADER_STAGE_FRAGMENT_BIT: stageStr = "frag"; break;
			case VK_SHADER_STAGE_COMPUTE_BIT: stageStr = "comp"; break;
			default: stageStr = "unknown"; break;
		}
		parser.parseShaderInput(shader.create(mainDevice.logicalDevice, "Shaders/" + shaderFileName + "." + stageStr + ".spv", stage), layouts);
	}

	void VulkanRenderer::loadShader(const std::string& shaderFileName, std::unordered_map<VkDescriptorType, uint32_t>& descriptorTypes)
	{
		Shader shader;
		shader.mId = static_cast<uint32_t>(mShaders.size());
		ShaderInputParser parser;
		std::vector<VulkanDescriptorSetLayoutInfo> layoutInfos;
		loadShaderStage(parser, shader.mVertexShader, shaderFileName, VK_SHADER_STAGE_VERTEX_BIT, layoutInfos);
		loadShaderStage(parser, shader.mFragmentShader, shaderFileName, VK_SHADER_STAGE_FRAGMENT_BIT, layoutInfos);
		loadShaderStage(parser, shader.mComputeShader, shaderFileName, VK_SHADER_STAGE_COMPUTE_BIT, layoutInfos);
		loadShaderStage(parser, shader.mRayGenShader, shaderFileName, VK_SHADER_STAGE_RAYGEN_BIT_KHR, layoutInfos);
		loadShaderStage(parser, shader.mRayMissShader, shaderFileName, VK_SHADER_STAGE_MISS_BIT_KHR, layoutInfos);
		loadShaderStage(parser, shader.mRayClosestHitShader, shaderFileName, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, layoutInfos);

		for(const auto& layoutInfo : layoutInfos)
		{
			uint32_t dslId = createDescriptorSetLayout(layoutInfo);
			shader.mDSLs.push_back(dslId);
			for(const auto dt : layoutInfo.mDescriptorTypes)
			{
                if(descriptorTypes.find(dt) == descriptorTypes.end())
                {
                    descriptorTypes[dt] = 0;
                }
				descriptorTypes[dt]++;
			}
		}

		shader.mName = shaderFileName;

		mShaders.push_back(shader);
	}

	void VulkanRenderer::loadUsedShaders()
	{
		mModelMatrixPCR.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//Shader stage push constant will go to
		mModelMatrixPCR.offset = 0;
		mModelMatrixPCR.size = sizeof(mat4);

        mLightingPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mLightingPCR.offset = sizeof(mat4);
		mLightingPCR.size = sizeof(Lighting);

        mNearFarPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mNearFarPCR.offset = 0;
		mNearFarPCR.size = sizeof(vec2);
		
		//Force to load fog shader
		mFogShaderId = addShader("fog");
        std::unordered_map<VkDescriptorType, uint32_t> descriptorTypes;

		for(const auto& shaderFileName : mShaderFileNames)
		{
			mShaderMetaDatum.push_back(getShaderMetaData(shaderFileName));
			loadShader(shaderFileName, descriptorTypes);
		}

		VulkanDescriptorPoolKey descriptorPoolKey;
        for(const auto& [descriptorType, count] : descriptorTypes)
		{
            descriptorPoolKey.mPoolSizes.push_back({ descriptorType, count * MAX_FRAME_DRAWS });
		}
        
		mSharedDescriptorPoolId = createDescriptorPool(descriptorPoolKey);
	}

	void VulkanRenderer::bindPipeline(const VulkanPipeline& pipeline)
	{
		vkCmdBindPipeline(
				pipeline.isCompute() ? mComputeCommandBuffers[mImageIndex].mCommandBuffer : mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
				pipeline.mBindPoint,
				pipeline.mPipeline);
	}

	void VulkanRenderer::bindVertexBuffers(const VkBuffer* buffers, uint32_t count, VkDeviceSize* offsets, VkPipelineBindPoint pipelineBindPoint)
	{
		vkCmdBindVertexBuffers(
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ?
				mComputeCommandBuffers[mImageIndex].mCommandBuffer :
				mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
			0, count, buffers, offsets);
	}

	void VulkanRenderer::bindIndexBuffer(const VkBuffer buffer, VkPipelineBindPoint pipelineBindPoint)
	{
		vkCmdBindIndexBuffer(
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ?
				mComputeCommandBuffers[mImageIndex].mCommandBuffer :
				mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
			buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	const VulkanBuffer* VulkanRenderer::getVertexBuffer(const uint32_t meshId) const
	{
		const auto found = mMeshToVertexBufferMap.find(meshId);
		const VulkanBuffer* result = found == mMeshToVertexBufferMap.end() ? nullptr :
			mBufferManager.getBuffer(found->second);

		return result;
	}

	const VulkanBuffer* VulkanRenderer::getIndexBuffer(const uint32_t meshId) const
	{
		const auto found = mMeshToIndexBufferMap.find(meshId);
		const VulkanBuffer* result = found == mMeshToIndexBufferMap.end() ? nullptr :
			mBufferManager.getBuffer(found->second);

		return result;
	}

	void VulkanRenderer::recordCommands(const Camera& camera, const Light& light)
	{
		mGraphicsCommandBuffers[mImageIndex].begin();
		VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[mImageIndex].mCommandBuffer;
		mRenderPass.begin(mFrameBuffers[mImageIndex].mFrameBuffer, mSwapChain.mSwapChainExtent,
			commandBuffer, mClearColor);

		for(int32_t i = 0; i < mSubPassesCount; i++)
		{
			renderSubPass(i, camera, light);

			if(i < mSubPassesCount - 1)
			{
				vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			}
		}

		mRenderPass.end(commandBuffer);

		recordSceneCommands(camera, light, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 0);

		mGraphicsCommandBuffers[mImageIndex].end();
	}

	void VulkanRenderer::getPhysicalDevice()
	{
		//Enumerate physical devices the VkInstance can access
		uint32_t deviceCount = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr));
		//If no devices available, then Vulkan is not supported!
		if (deviceCount == 0)
		{
			throw std::runtime_error("Can't find GPU that supports Vulkan Instance!");
		}
		//Get list of physical devices
		std::vector<VkPhysicalDevice> deviceList(deviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &deviceCount, deviceList.data()));

		for (const auto& device : deviceList)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			LOG_INFO("Device Id: {}", deviceProperties.deviceID);
			LOG_INFO("Device Name: {}", deviceProperties.deviceName);
			LOG_INFO("Device Type: {}", deviceProperties.deviceType);
			LOG_INFO("API Version: {}", deviceProperties.apiVersion);
			LOG_INFO("Driver Version: {}", deviceProperties.driverVersion);
			LOG_INFO("------------------------------------------");

			mQueueFamilies = getQueueFamilies(device, mSurface);
			for(const auto& queueFamily : mQueueFamilies)
			{
				if(queueFamily.isFullySupported() && checkDeviceSuitable(device))
				{
					mainDevice.physicalDevice = device;
					break;
				}
			}
			
			if(mainDevice.physicalDevice != VK_NULL_HANDLE)
			{
				break;
			}
		}

		if(mainDevice.physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("getPhysicalDevice(): No suitable GPU found for required extensions/features!");
		}

		//Get properties of device
		/*VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

		minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;*/
	}

	void VulkanRenderer::allocateDynamicBufferTransferSpace()
	{
		//Calculate alignment of model data
		/*modelUniformAlignment = (sizeof(ModelMatrix) + minUniformBufferOffset - 1)
			& ~(minUniformBufferOffset - 1);

		//Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
		modetTransferSpace = (ModelMatrix*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);*/
	}

	void VulkanRenderer::cleanupSwapChain()
	{
		mSwapChain.destroy(mainDevice.logicalDevice);
		cleanupSwapChainFrameBuffers();
		cleanupInputDescriptorPool();
		cleanupInputDescriptorSetLayout();
		cleanupSwapchainImagesSemaphores();
	}

	void VulkanRenderer::createSwapChain()
	{
		mSwapChain.create(mWindow, mainDevice, mGraphicsQueueFamilyId,
			mPresentationQueueFamilyId, mSurface);
		createSwapChainFrameBuffers();
		createInputDescriptorPool();
		createInputDescriptorSetLayout();
		allocateInputDescriptorSets();

		createSwapchainImagesSemaphores();
	}

	void VulkanRenderer::recreateSwapChain()
	{
		LOG_INFO("Recreate swapchain");

		int width = 0, height = 0;
		glfwGetFramebufferSize(mWindow, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(mWindow, &width, &height);
			glfwWaitEvents();
		}

		VK_CHECK(vkDeviceWaitIdle(mainDevice.logicalDevice));

		cleanupSwapChain();

        createSwapChain();

		LOG_INFO("Swapchain recreated");
	}

	bool VulkanRenderer::checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions)
	{
		//Need to get number of extensions to create array of correct size to hold extentions
		uint32_t extensionsCount = 0;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr));

		//Create a list of VkExtensionProperties using count
		std::vector<VkExtensionProperties> extensions(extensionsCount);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data()));

		//Check if given extenxtions are in list of available extensions
		for (const auto& checkExtension : *checkExtensions)
		{
			bool hasExtension = false;
			for (const auto& extension : extensions)
			{
				if (strcmp(checkExtension, extension.extensionName) == 0)
				{
					hasExtension = true;
					break;
				}
			}

			if (!hasExtension)
			{
				return false;
			}
		}

		return true;
	}

	bool VulkanRenderer::checkValidationLayerSupport() {
		uint32_t layerCount;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		std::vector<VkLayerProperties> availableLayers(layerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		//Get device extension count
		uint32_t extensionCount = 0;
		VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));

		//If no extensions found, return failure
		if (extensionCount == 0)
		{
			return false;
		}

		//Populate list of extensions
		std::vector<VkExtensionProperties> extensions(extensionCount);
		VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data()));

		for (const auto& deviceExtension : mRequestedDeviceExtensions)
		{
			bool hasExtension = false;
			for (const auto& extension : extensions)
			{
				if (strcmp(deviceExtension, extension.extensionName) == 0)
				{
					hasExtension = true;
					return true;
					break;
				}
			}

			if (!hasExtension)
			{
				return false;
			}
		}

		return true;
	}


	bool VulkanRenderer::checkDeviceFeaturesSupport()
	{
		bool result = true;
		for(auto& f : mDeviceFeaturesEnabled)
		{
			if(*f != VK_TRUE)
			{
				result = false;
				break;
			}
		}

		return result;
	}

	bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
	{
		/*
		//Information about the device itself (ID, name, type, Vendor, etc)
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);*/


		//Information about what device can do (geo shader, tess shaders, wide lines, etc.)

		//Extended features
		requestDeviceFeatures();
		
		vkGetPhysicalDeviceFeatures2(device, &mDeviceFeatures);

		bool featuresSupported = checkDeviceFeaturesSupport();

		bool extensionSupported = checkDeviceExtensionSupport(device);

		bool swapChainValid = false;
		if (extensionSupported)
		{
			SwapChainDetails swapChainDetails = mSwapChain.getSwapChainDetails(device, mSurface);
			swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
		}

		return extensionSupported && featuresSupported && swapChainValid && mDeviceFeatures.features.samplerAnisotropy;
	}

	void VulkanRenderer::createSwapchainImagesSemaphores()
	{
		mImageAvailable.resize(MAX_FRAME_DRAWS);
		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VK_CHECK(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &mImageAvailable[i]));
		}
	}

	void VulkanRenderer::createRenderFinishedSemaphores()
	{
		mRenderFinished.resize(MAX_FRAME_DRAWS);
		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VK_CHECK(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &mRenderFinished[i]));
		}
	}

	void VulkanRenderer::createComputeFinishedSemaphores()
	{
		mComputeFinished.resize(MAX_FRAME_DRAWS);
		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VK_CHECK(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &mComputeFinished[i]));
		}
	}

	void* VulkanRenderer::getSemaphoreHandle(VkSemaphore semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType)
	{
		#ifdef _WIN64
			HANDLE handle;

			VkSemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfoKHR = {};
			semaphoreGetWin32HandleInfoKHR.sType =
				VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
			semaphoreGetWin32HandleInfoKHR.pNext = NULL;
			semaphoreGetWin32HandleInfoKHR.semaphore = semaphore;
			semaphoreGetWin32HandleInfoKHR.handleType = handleType;

			PFN_vkGetSemaphoreWin32HandleKHR fpGetSemaphoreWin32HandleKHR;
			fpGetSemaphoreWin32HandleKHR =
				(PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(
					mainDevice.logicalDevice, "vkGetSemaphoreWin32HandleKHR");
			if(!fpGetSemaphoreWin32HandleKHR)
			{
				throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
			}
			VK_CHECK(fpGetSemaphoreWin32HandleKHR(mainDevice.logicalDevice, &semaphoreGetWin32HandleInfoKHR, &handle));

			return (void*)handle;
		#else
			int fd;

			VkSemaphoreGetFdInfoKHR semaphoreGetFdInfoKHR = {};
			semaphoreGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
			semaphoreGetFdInfoKHR.pNext = NULL;
			semaphoreGetFdInfoKHR.semaphore = semaphore;
			semaphoreGetFdInfoKHR.handleType = handleType;

			PFN_vkGetSemaphoreFdKHR fpGetSemaphoreFdKHR;
			fpGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(
				mainDevice.logicalDevice, "vkGetSemaphoreFdKHR");
			if(!fpGetSemaphoreFdKHR)
			{
				throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
			}
			VK_CHECK(fpGetSemaphoreFdKHR(mainDevice.logicalDevice, &semaphoreGetFdInfoKHR, &fd));

			return (void*)(uintptr_t)fd;
		#endif /* _WIN64 */
	}

	VkSemaphore VulkanRenderer::createExternalSemaphore(VkExternalSemaphoreHandleTypeFlagBits handleType)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkExportSemaphoreCreateInfoKHR exportSemaphoreCreateInfo = {};
		exportSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;
		exportSemaphoreCreateInfo.handleTypes = handleType;
		semaphoreInfo.pNext = &exportSemaphoreCreateInfo;

		VkSemaphore result;
		VK_CHECK(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreInfo, nullptr, &result));
		mSemaphores.push_back(result);

		return result;
	}

	void VulkanRenderer::createExternalSemaphores()
	{
		mExternalWaitSemaphore = createExternalSemaphore(getDefaultSemaphoreHandleType());
		mExternalSignalSemaphore = createExternalSemaphore(getDefaultSemaphoreHandleType());
	}

	void VulkanRenderer::createDrawFences()
	{
		mDrawFences.resize(MAX_FRAME_DRAWS);
		//Fence creation information
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VK_CHECK(vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &mDrawFences[i]));
		}
	}

	void VulkanRenderer::createComputeFences()
	{
		mComputeFences.resize(MAX_FRAME_DRAWS);
		//Fence creation information
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			VK_CHECK(vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &mComputeFences[i]));
		}
	}

	void VulkanRenderer::createTransferSynchronisation()
	{
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreInfo, nullptr, &mTransferCompleteSemaphore));

		VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK(vkCreateFence(mainDevice.logicalDevice, &fenceInfo, nullptr, &mTransferFence));
	}

	void VulkanRenderer::initRayTracing()
	{
		// SPIRV 1.4 requires Vulkan 1.1
		setApiVersion(VK_API_VERSION_1_1);

		mRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProperties{};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties.pNext = &mRayTracingPipelineProperties;
		vkGetPhysicalDeviceProperties2(mainDevice.physicalDevice, &deviceProperties);

		mAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures{};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures.pNext = &mAccelerationStructureFeatures;
		vkGetPhysicalDeviceFeatures2(mainDevice.physicalDevice, &deviceFeatures);
	}

	void VulkanRenderer::createSynchronisation()
	{
		LOG_INFO("Create synchronisation objects");

		createSwapchainImagesSemaphores();
		createRenderFinishedSemaphores();
		createComputeFinishedSemaphores();
		createDrawFences();
		createComputeFences();
		createTransferSynchronisation();
		if(mHasExternalResources)
		{
			createExternalSemaphores();
		}

		LOG_INFO("Synchronisation objects created");
	}

    void VulkanRenderer::setViewport(const BoundingBox2D& v)
    {
		VkViewport viewport{};
		viewport.x = v.mMin.x;
		viewport.y = v.mMin.y;
		viewport.width = v.mMax.x - v.mMin.x;
		viewport.height = v.mMax.y - v.mMin.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(mGraphicsCommandBuffers[mImageIndex].mCommandBuffer, 0, 1, &viewport);
    }

    void VulkanRenderer::setScissor(const BoundingBox2D& scissorRect)
    {
		VkRect2D scissor{};
		scissor.offset =
		{
			max(0, static_cast<int>(scissorRect.mMin.x)),
			max(0, static_cast<int>(scissorRect.mMin.y))
		};
		const auto size = scissorRect.getSize();
		scissor.extent.width = max(0, static_cast<int>(size.x));
		scissor.extent.height = max(0, static_cast<int>(size.y));
		vkCmdSetScissor(mGraphicsCommandBuffers[mImageIndex].mCommandBuffer, 0, 1, &scissor);
    }

	void VulkanRenderer::loadImages()
	{
		//std::cout << "render tid: " << std::this_thread::get_id() << std::endl;
		mStatistics.startMeasure("load images", static_cast<float>(Timer::getInstance().getTime()));
		mTextureManager.loadImages(
			[this](int imageIndex, int count)
			{
				if(imageIndex == count - 1)
				{
					this->mStatistics.stopMeasure("load images", static_cast<float>(Timer::getInstance().getTime()));
					this->mStatistics.print();
				}

				this->requestRedraw();
			},
			mThreadPool
		);

		/*uint32_t cnt = mTextureManager.getImagesCount();
		for(uint32_t i = 0; i < cnt; i++)
		{
			auto* image = mTextureManager.getImage(i);
			//load default texture in main thread
			if(i == 0 && image != nullptr)
			{
				image->load();
				mTextureManager.getDescriptorSet(mainDevice, mTransferQueueFamilyId,
					mGraphicsQueueFamilyId, mGraphicsQueue, mGraphicsCommandPool, i);
			}
			else
			{
				mThreadPool.enqueue
				(
					[this, i, cnt]
					{
						std::lock_guard<std::mutex> lock(gRenderMutex);

						auto* image = this->mTextureManager.getImage(i);
						if(image != nullptr)
						{
							image->load();
						}

						if(i == cnt - 1)
						{
							this->mStatistics.stopMeasure("load images", static_cast<float>(Timer::getInstance().getTime()));
							this->mStatistics.print();
						}
						
						this->requestRedraw();
					}
				);
			}
		}*/
	}

	void VulkanRenderer::loadMeshes()
	{
		for(auto& meshModel : mMeshModels)
		{
			for(uint32_t i = 0; i < meshModel->getMeshCount(); i++)
			{
				const auto& mesh = meshModel->getMesh(i);
				uint32_t meshId = mesh->getId();
				const Material& material = mMaterials[mesh->getMaterialId()];
				bool useCompute = mesh->getComputeShaderId() != std::numeric_limits<uint32_t>::max();
				uint32_t usage = 0;
				if(useCompute)
				{
					usage = usage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				}

				if(mesh->getVertexCount() > 0)
				{
					const void* vertexData = mesh->getVertexData();
					mMeshToVertexBufferMap[meshId] = static_cast<uint32_t>(mBufferManager.mBuffers.size());
					uint32_t vertexBufferSize = mesh->getVertexCount() * mesh->getVertexSize();
					mBufferManager.createBuffer(
						mainDevice, mTransferQueue, mTransferCommandPool,
						static_cast<VkBufferUsageFlagBits>(usage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexData, vertexBufferSize);
				}

				if(mesh->getIndexCount() > 0)
				{
					const void* indexData = mesh->getIndexData();
					uint32_t indexBufferSize = mesh->getIndexCount() * sizeof(uint32_t);
					mMeshToIndexBufferMap[meshId] = static_cast<uint32_t>(mBufferManager.mBuffers.size());
					mBufferManager.createBuffer(
						mainDevice, mTransferQueue, mTransferCommandPool,
						static_cast<VkBufferUsageFlagBits>(usage | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
						VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexData, indexBufferSize);
				}
			}
		}
	}

	void VulkanRenderer::createBarrier(VkBuffer buffer, VkPipelineBindPoint pipelineBindPoint)
	{
		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;   // Compute shader writes
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;    // Vertex shader reads
		barrier.srcQueueFamilyIndex = mComputeQueueFamilyId;
		barrier.dstQueueFamilyIndex = mGraphicsQueueFamilyId;
		barrier.buffer = buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? mComputeCommandBuffers[mImageIndex].mCommandBuffer
				: mGraphicsCommandBuffers[mImageIndex].mCommandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			0,
			0, nullptr,
			1, &barrier,
			0, nullptr
		);
	}
	
	ShaderMetaDatum VulkanRenderer::getShaderMetaData(const std::string& shaderFileName)
	{
		ShaderMetaDatum result;

		auto commonPushConstantsCallback =
			[this](const Mesh::Ptr& mesh, const mat4& modelMatrix, const Camera& camera, const Light& light, VkPipelineLayout pipelineLayout, uint32_t instanceId)
			{
				if (mesh != nullptr)
				{
					pushConstants(mModelMatrixPCR, &modelMatrix[0], pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);

					fillLightingPushConstant(mesh, modelMatrix, camera, light, mLighting);
					//std::cout << "mat3 size: " << sizeof(mat3) << std::endl;
					//std::cout << "Lighting.normalMatrix: " << offsetof(Lighting, normalMatrix) << std::endl;
					//std::cout << "Lighting.cameraEye: " << offsetof(Lighting, cameraEye) << std::endl;
					//std::cout << "Lighting.lightPos: " << offsetof(Lighting, lightPos) << std::endl;
					pushConstants(mLightingPCR, &mLighting, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
				}
			};

		auto commonBindDescriptorSetsCallback =
			[this](const Mesh::Ptr& mesh, const Material& material, VkPipelineLayout pipelineLayout, uint32_t instanceId)
			{
				//Bind all textures of material
				std::vector<VkDescriptorSet> descriptorSets;
				descriptorSets.push_back(mUniformDescriptorSets[mImageIndex].mDescriptorSet);
			
				for(const auto textureId : material.mTextureIds)
				{
					mTextureManager.getTextureInfo
					descriptorSets.push_back(getSamplerDS(textureId.second, VK_PIPELINE_BIND_POINT_GRAPHICS));
				}

				bindDescriptorSets(mesh->getDescriptorSets(), pipelineLayout, descriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS);
			};

		if(shaderFileName == "pbr")
		{
			ShaderMetaData md;
			md.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			md.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout,
				//Color texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
				//Normals texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
				//Matallic texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
			};
			md.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			md.mPushConstantsCallback = commonPushConstantsCallback;
			md.mBindDescriptorSetsCallback =
			[this, md](const Mesh::Ptr& mesh, const Material& material, VkPipelineLayout pipelineLayout, uint32_t instanceId)
			{
				std::vector<VkDescriptorSet> descriptorSets;
				throw::std::runtime_error("Not all descriptorsets passet to rt shader");
				for(const auto textureId : material.mTextureIds)
				{
					descriptorSets.push_back(getSamplerDS(textureId.second, VK_PIPELINE_BIND_POINT_GRAPHICS));
				}

				bindDescriptorSets(pipelineLayout, descriptorSets, VK_PIPELINE_BIND_POINT_GRAPHICS);
			};
			md.mDepthTestEnabled = true;
			md.mVertexSize = sizeof(Vertex);
			md.mSubPassIndex = 0;

			result.push_back(md);
		}
		else if(shaderFileName == "normalMap")
		{
			ShaderMetaData md;

			md.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			md.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout,
				//Color texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
				//Normals texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
			};
			md.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			md.mPushConstantsCallback = commonPushConstantsCallback;
			md.mBindDescriptorSetsCallback = commonBindDescriptorSetsCallback;
			md.mDepthTestEnabled = true;
			md.mVertexSize = sizeof(Vertex);
			md.mSubPassIndex = 0;

			result.push_back(md);
		}
		else if(shaderFileName == "textured")
		{
			ShaderMetaData md;

			md.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			md.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout,
				//Color texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
			};
			md.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			md.mPushConstantsCallback = commonPushConstantsCallback;
			md.mBindDescriptorSetsCallback = commonBindDescriptorSetsCallback;
			md.mDepthTestEnabled = true;
			md.mVertexSize = sizeof(Vertex);
			md.mSubPassIndex = 0;
			
			result.push_back(md);
		}
		else if(shaderFileName == "colored")
		{
			ShaderMetaData md;

			md.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			md.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout
			};
			md.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			md.mPushConstantsCallback = commonPushConstantsCallback;
			md.mBindDescriptorSetsCallback = commonBindDescriptorSetsCallback;
			md.mDepthTestEnabled = true;
			md.mVertexSize = sizeof(Vertex);
			md.mSubPassIndex = 0;
			
			result.push_back(md);
		}
		else if(shaderFileName == "fog")
		{
			ShaderMetaData md;

			md.mDescriptorSetLayouts = {mInputDescriptorSetLayout.mDescriptorSetLayout, mDepthDescriptorSetLayout.mDescriptorSetLayout};
			md.mPushConstantRanges = {mNearFarPCR};
			md.mPushConstantsCallback =
				[this](const Mesh::Ptr& mesh, const mat4& modelMatrix, const Camera& camera,
					const Light& light, VkPipelineLayout pipelineLayout, uint32_t instanceId)
				{
					vec2 nearFar(camera.mNear, camera.mFar);
					pushConstants(mNearFarPCR, &nearFar, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS);
				};
			md.mDepthTestEnabled = false;
			md.mVertexSize = 0;
			md.mSubPassIndex = 1;

			result.push_back(md);
		}

		if(result.empty() && mShaderMetaDataProvider != nullptr)
		{
			result = mShaderMetaDataProvider->getShaderMetaData(shaderFileName);
		}
		
		return result;
	}

	void VulkanRenderer::setShaderMetaDataProvider(ShaderMetaDataProvider* provider)
	{
		mShaderMetaDataProvider = provider;
	}

	void VulkanRenderer::addMaterial(Material& material)
	{
		std::string shaderFileName = material.mShaderFileName;
		if(shaderFileName.empty())
		{
			if(material.hasTextureTypes({aiTextureType_BASE_COLOR, aiTextureType_NORMALS, aiTextureType_METALNESS}))
			{
				shaderFileName = "pbr";
			}
			else if(material.hasTextureTypes({aiTextureType_DIFFUSE, aiTextureType_NORMALS}))
			{
				shaderFileName = "normalMap";
			}
			else if(material.hasTextureTypes({aiTextureType_DIFFUSE}))
			{
				shaderFileName = "textured";
			}
			else
			{
				shaderFileName = mDefaultShaderFileName;
			}
		}

		if(!shaderFileName.empty())
		{
			material.mShaderId = addShader(shaderFileName);
		}
		mMaterials.push_back(material);
	}

	int VulkanRenderer::addShader(const std::string& shaderFileName)
	{
		int result = getIndexOf(mShaderFileNames, shaderFileName);
		if (result == -1)
		{
			result = static_cast<uint32_t>(mShaderFileNames.size());
			mShaderFileNames.push_back(shaderFileName);
		}
		return result;
	}

	MeshModel::Ptr& VulkanRenderer::createMeshModel(std::string modelFile,
		const std::vector<aiTextureType>& texturesLoadTypes)
	{
		//Import model scene
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(modelFile,
			aiProcess_Triangulate | aiProcess_CalcTangentSpace |
			aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType);
		if (!scene)
		{
			throw::std::runtime_error("Failed to load model " + modelFile + ". Error: " + importer.GetErrorString());
		}

		//Load materials
		uint32_t materialsOffset = static_cast<uint32_t>(mMaterials.size());
		for (uint32_t m = 0; m < scene->mNumMaterials; m++)
		{
			aiMaterial* externalMaterial = scene->mMaterials[m];
			Material material;
			externalMaterial->Get(AI_MATKEY_SHININESS, material.mShininess);
			if(areEqual(material.mShininess, 0.0f))
			{
				material.mShininess = mDefaultShininess;
			}

			//Look at textures we are interested in
			for(uint32_t i = 1; i < aiTextureType_UNKNOWN; i++)
			{
				aiTextureType textureType = static_cast<aiTextureType>(i);
				auto foundIt = std::find(texturesLoadTypes.begin(),
					texturesLoadTypes.end(), textureType);
				bool found = foundIt != texturesLoadTypes.end();
				if(found)
				{
					//Get texture file path
					aiString path;
					if (externalMaterial->GetTexture(textureType, 0, &path) == AI_SUCCESS)
					{
						const auto foundTexId = addTexture(path.C_Str(), false);
						material.mTextureIds[textureType] = foundTexId;
					}
				}
			}

			addMaterial(material);
		}

		//Load all meshes
		MeshModel::MeshList modelMeshes = MeshModel::loadNode(
			scene->mRootNode, scene, mSceneBoundingBox, materialsOffset);

		return addMeshModel(modelMeshes);
	}

	void VulkanRenderer::requestRedraw()
	{
		mNeedRedraw = 3;
		//std::cout << "[" << mImageIndex << "]" << "request redraw" << std::endl;
	}

	void VulkanRenderer::resetRedrawRequest()
	{
		mNeedRedraw = std::max(0, mNeedRedraw - 1);
	}

	bool VulkanRenderer::needRedraw()
	{
		#if defined(REDRAW_EVERY_FRAME)
			return true;
		#else
			return mNeedRedraw > 0;
		#endif
	}

    void VulkanRenderer::readFromGPUMemory(VkDeviceMemory memory, void* dstBuffer, size_t size)
	{
        std::vector<VkFence> fences = {mComputeFences[mCurrentFrame]};
		VK_CHECK(vkWaitForFences(mainDevice.logicalDevice, fences.size(), fences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max()));
		void* mappedData;
		VK_CHECK(vkMapMemory(mainDevice.logicalDevice, memory, 0, size, 0, &mappedData));
		memcpy(dstBuffer, mappedData, size);
		//Unamp vertex buffer memory
        vkUnmapMemory(mainDevice.logicalDevice, memory);
	}
	
	void* VulkanRenderer::getMemHandle(VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagBits handleType)
	{
		#ifdef _WIN64
			HANDLE handle = 0;

			VkMemoryGetWin32HandleInfoKHR vkMemoryGetWin32HandleInfoKHR = {};
			vkMemoryGetWin32HandleInfoKHR.sType =
				VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
			vkMemoryGetWin32HandleInfoKHR.pNext = NULL;
			vkMemoryGetWin32HandleInfoKHR.memory = memory;
			vkMemoryGetWin32HandleInfoKHR.handleType = handleType;

			PFN_vkGetMemoryWin32HandleKHR fpGetMemoryWin32HandleKHR;
			fpGetMemoryWin32HandleKHR =
				(PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(
					mainDevice.logicalDevice, "vkGetMemoryWin32HandleKHR");
			if(!fpGetMemoryWin32HandleKHR)
			{
				throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
			}
			VK_CHECK(fpGetMemoryWin32HandleKHR(mainDevice.logicalDevice, &vkMemoryGetWin32HandleInfoKHR, &handle));

			return (void*)handle;
		#else
			int fd = -1;

			VkMemoryGetFdInfoKHR vkMemoryGetFdInfoKHR = {};
			vkMemoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
			vkMemoryGetFdInfoKHR.pNext = NULL;
			vkMemoryGetFdInfoKHR.memory = memory;
			vkMemoryGetFdInfoKHR.handleType = handleType;

			PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR;
			fpGetMemoryFdKHR =
				(PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(mainDevice.logicalDevice, "vkGetMemoryFdKHR");
			if(!fpGetMemoryFdKHR)
			{
				throw std::runtime_error("Failed to retrieve vkGetMemoryWin32HandleKHR!");
			}
			VK_CHECK(fpGetMemoryFdKHR(mainDevice.logicalDevice, &vkMemoryGetFdInfoKHR, &fd));

			return (void*)(uintptr_t)fd;
		#endif /* _WIN64 */
	}
}