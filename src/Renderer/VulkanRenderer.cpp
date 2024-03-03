#include "Renderer/VulkanRenderer.hpp"

#include "Camera.hpp"
#include "Light.hpp"
#include "Timer.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanShader.hpp"
#include "config.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <limits>
#include <stdexcept>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace fre
{
	//shader input
	struct Lighting
	{
		glm::vec4 cameraEye;
		glm::vec4 lightPos;
		glm::vec4 lightColor;
		glm::mat4 normalMatrix;
	};

	VulkanRenderer::VulkanRenderer()
	: mThreadPool(std::thread::hardware_concurrency())
	{
		std::cout << "Number of concurent threads: " << std::thread::hardware_concurrency() << std::endl;
		mTextureFileNames.push_back("default.jpg");
	}

	VulkanRenderer::~VulkanRenderer()
	{
	}

	int VulkanRenderer::create(GLFWwindow* newWindow)
	{
		window = newWindow;

		try {
			createInstance();
			createSurface();
			getPhysicalDevice();
			createLogicalDevice();
			mSwapChain.create(window, mainDevice, mGraphicsQueueFamilyId,
				mPresentationQueueFamilyId, surface);
			mRenderPass.create(mainDevice, mSwapChain.mSwapChainImageFormat);
			createSwapChainFrameBuffers();
			createUniformDescriptorPool();
			createInputDescriptorPool();
			createUniformDescriptorSetLayout();
			createInputDescriptorSetLayout();
			mTextureManager.create(mainDevice.logicalDevice);
			//allocateDynamicBufferTransferSpace();
			createUniformBuffers();
			allocateUniformDescriptorSets();
			allocateInputDescriptorSets();
			loadUsedShaders();
			createGraphicsPipelines();
			createCommandPools();
			createCommandBuffers();
			createSynchronisation();

			loadImages();
			loadMeshes();

			createUI();
		}
		catch (std::runtime_error& e)
		{
			printf("ERROR: %s\n", e.what());
			return EXIT_FAILURE;
		}

		return 0;
	}

    void VulkanRenderer::destroy()
    {
		//Wait until no actions being run on device before destroying
		vkDeviceWaitIdle(mainDevice.logicalDevice);

		cleanupUI();

		//_aligned_free(modetTransferSpace);

		mBufferManager.destroy(mainDevice.logicalDevice);

		cleanupUniformDescriptorPool();
		cleanupInputDescriptorPool();
		cleanupUIDescriptorPool();

		cleanupInputDescriptorSetLayout();
		cleanupUniformDescriptorSetLayout();

		mTextureManager.destroy(mainDevice.logicalDevice);

		mSwapChain.destroy(mainDevice.logicalDevice);
		cleanupSwapChainFrameBuffers();

		for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
		{
			vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);
			
			//vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
			//vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
		}

		cleanupSwapchainImagesSemaphores();
		cleanupRenderFinishedSemaphores();
		cleanupDrawFences();
		cleanupTransferSynchronisation();

		vkDestroyCommandPool(mainDevice.logicalDevice, mGraphicsCommandPool, nullptr);
		if(mGraphicsCommandPool != mTransferCommandPool)
		{
			vkDestroyCommandPool(mainDevice.logicalDevice, mTransferCommandPool, nullptr);
		}
		
		cleanupGraphicsPipelines(mainDevice.logicalDevice);

		mRenderPass.destroy(mainDevice.logicalDevice);
		
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(mainDevice.logicalDevice, nullptr);
		vkDestroyInstance(instance, nullptr);
		
		mThreadPool.destroy();
	}

	void VulkanRenderer::addMeshModel(const MeshModel& meshModel)
	{
		mMeshModels.push_back(meshModel);
	}

	MeshModel* VulkanRenderer::getMeshModel(int modelId)
	{
		MeshModel* result = nullptr;

		if (modelId < mMeshModels.size())
		{
			result = &mMeshModels[modelId];
		}

		return result;
	}

	void VulkanRenderer::draw(const Camera& camera, const Light& light)
	{
		//Wait for given fence to signal (open) from last draw before continuing
		vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint32_t>::max());

		uint32_t imageIndex;
		//Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		VkResult result = vkAcquireNextImageKHR(mainDevice.logicalDevice, mSwapChain.mSwapChain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS/* && result != VK_SUBOPTIMAL_KHR*/)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		else
		{
			// -- GET NEXT IMAGE--
			//Manually reset (close) fences
			vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

			recordCommands(imageIndex, camera, light);

			updateUniformBuffers(imageIndex, camera);

			// -- SUBMIT COMMAND BUFFER TO RENDER --
			//Queue submission info
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;		//Number of semaphores to wait on
			submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];	//List of samephores to wait on
			VkPipelineStageFlags waitStages[] = {
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};
			submitInfo.pWaitDstStageMask = waitStages;	//Stages to check semaphores at
			submitInfo.commandBufferCount = 1;	//Number of command buffers to submit
			VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[imageIndex].mCommandBuffer;
			submitInfo.pCommandBuffers = &commandBuffer;	//Command buffer to submit
			submitInfo.signalSemaphoreCount = 1;	//Number of semaphores to signal
			submitInfo.pSignalSemaphores = &renderFinished[currentFrame];	//Semaphore to signal wen command buffer finishes

			result = vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to submit Command Buffer to Queue!");
			}

			// -- PRESENT RENDERED IMAGE TO SCREEN --
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;	//Number of semaphores to wait on
			presentInfo.pWaitSemaphores = &renderFinished[currentFrame];	//Semaphores to wait on
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &mSwapChain.mSwapChain;	//Swapchains to present images to
			presentInfo.pImageIndices = &imageIndex;	//Indices of images in swapchain to present

			result = vkQueuePresentKHR(mPresentationQueue, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
			{
				framebufferResized = false;
				recreateSwapChain();
			}
			else if (result != VK_SUCCESS)
			{
				throw std::runtime_error("failed to present swap chain image!");
			}

			//Get next frame
			currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
		}
	}

	void VulkanRenderer::drawUI(uint32_t imageIndex)
	{
		ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		{
			ImGui::Begin("PBR example");
			ImGui::ColorEdit3("Background color", (float*)&mClearColor.r);

			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
		//mGraphicsCommandBuffers[imageIndex].begin();
        ImGui_ImplVulkan_RenderDrawData(draw_data, mGraphicsCommandBuffers[imageIndex].mCommandBuffer);
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
	}

	void VulkanRenderer::cleanupUIDescriptorPool()
	{
		mUIDescriptorPool.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupInputDescriptorSetLayout()
	{
		mInputDescriptorSetLayout.destroy(mainDevice.logicalDevice);
	}

	void VulkanRenderer::cleanupUniformDescriptorSetLayout()
	{
		mUniformDescriptorSetLayout.destroy(mainDevice.logicalDevice);
	}

    void VulkanRenderer::cleanupSwapchainImagesSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
		}
    }

    void VulkanRenderer::cleanupRenderFinishedSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
		}
    }
    
	void VulkanRenderer::cleanupDrawFences()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
		}
    }

	void VulkanRenderer::cleanupTransferSynchronisation()
	{
		vkDestroySemaphore(mainDevice.logicalDevice, mTransferCompleteSemaphore, nullptr);
		vkDestroyFence(mainDevice.logicalDevice, mTransferFence, nullptr);
	}

	void VulkanRenderer::cleanupUI()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

    void VulkanRenderer::setFramebufferResized(bool resized)
    {
		framebufferResized = resized;
    }

	void VulkanRenderer::setClearColor(const glm::vec4& clearColor)
	{
		mClearColor = clearColor;
	}

    void VulkanRenderer::createInstance()
    {
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available!");
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
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//Creation information for VkInstance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		//Create list of extentions
		std::vector<const char*> instanceExtentions = std::vector<const char*>();
		//Setup extentions instance will use
		uint32_t glfwExtentionsCount = 0;
		//Extentions are passed as array of cstrings
		const char** glfwExtentions;
		glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionsCount);
		for (size_t i = 0; i < glfwExtentionsCount; i++)
		{
			instanceExtentions.push_back(glfwExtentions[i]);
		}

		//Check instance extentions supported
		if (!checkInstanceExtentionsSupport(&instanceExtentions))
		{
			throw std::runtime_error("VkInstance does not support required extentions!");
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtentions.size());
		createInfo.ppEnabledExtensionNames = instanceExtentions.data();
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		//Create instance
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Vulkan instance!");
		}
	}

	void VulkanRenderer::createLogicalDevice()
	{
		//Vector for queue creation information, and set for family indices
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		
		int8_t graphicsQueueId = -1;
		int8_t presentationQueueId = -1;
		int8_t transferQueueId = -1;
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
				}
				transferQueueId = queueCreateInfo.queueCount - 1;
				mTransferQueueFamilyId = queueFamily.mId;
			}
			float priority = 1.0f;
			//Vulkan needs to know how to handle multiple queues, so decide priority (1 - highest priority)
			queueCreateInfo.pQueuePriorities = &priority;

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
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtentions.size());	//Number of enabled logical device extensions
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtentions.data();
		
		//Physical device features the logical device will be using
		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
		physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

		//Create logical device for the given physical device
		VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		//Queues are created at the same time as the device
		//So we want hande to queues
		//From given logical device, of given Queue Family, of given queue index (0 since only one queue), place reference in given VkQueue
		vkGetDeviceQueue(mainDevice.logicalDevice, mGraphicsQueueFamilyId, graphicsQueueId, &mGraphicsQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, mPresentationQueueFamilyId, presentationQueueId, &mPresentationQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, mTransferQueueFamilyId, transferQueueId, &mTransferQueue);
	}

	void VulkanRenderer::createSwapChainFrameBuffers()
	{
		mFrameBuffers.resize(mSwapChain.mSwapChainImages.size());
		
		for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
		{
			VulkanAttachment colorAttachment;
			VulkanAttachment depthAttachment;
			
			colorAttachment.create(mainDevice, AK_COLOR, mSwapChain.mSwapChainExtent);
			depthAttachment.create(mainDevice, AK_DEPTH, mSwapChain.mSwapChainExtent);

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
	}

	void VulkanRenderer::createSurface()
	{
		//Create surface (creates a surface create info struct, runs the create surface function, returns result)
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a surface!");
		}
	}

	void VulkanRenderer::createUniformDescriptorPool()
	{
		mUniformDescriptorPool.create(
			mainDevice.logicalDevice,
			0,
			static_cast<uint32_t>(mSwapChain.mSwapChainImages.size()),
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER});
	}

	void VulkanRenderer::createInputDescriptorPool()
	{
		//pool for color and depth attachments
		mInputDescriptorPool.create(
			mainDevice.logicalDevice,
			0,
			MAX_OBJECTS,
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT});
	}

	void VulkanRenderer::createUIDescriptorPool()
	{
		//pool for color and depth attachments
		mUIDescriptorPool.create(
			mainDevice.logicalDevice,
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			MAX_OBJECTS,
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER});
	}

	void VulkanRenderer::createUniformDescriptorSetLayout()
	{
		//Uniforms layout
		mUniformDescriptorSetLayout.create(
			mainDevice.logicalDevice,
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
			VK_SHADER_STAGE_VERTEX_BIT);
	}

	void VulkanRenderer::createInputDescriptorSetLayout()
	{
		mInputDescriptorSetLayout.create(
			mainDevice.logicalDevice,
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT},
			VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	void VulkanRenderer::createGraphicsPipelines()
	{
		for(auto& shader : mShaders)
		{
			ShaderMetaData& shaderMetaData = mShaderMetaData[shader.mId];
			shader.mPipelineId = static_cast<uint32_t>(mPipelines.size());
			mPipelines.push_back(VulkanPipeline());
			auto& pipeline = mPipelines.back();
			pipeline.create(
				mainDevice.logicalDevice,
				{shader.mVertexShader, shader.mFragmentShader},
				shaderMetaData.mVertexSize,
				//no attributes for fog pass
				shaderMetaData.mVertexAttributes,
				shaderMetaData.mDepthTestEnabled ? VK_TRUE : VK_FALSE,
				shaderMetaData.mPolygonMode,
				mRenderPass.mRenderPass,
				shaderMetaData.mSubPassIndex,
				shaderMetaData.mDescriptorSetLayouts,
				shaderMetaData.mPushConstantRanges
			);
		}

		//2 sub passes each using 1 pipeline
        mSubPassesCount = 2;
	}

	void VulkanRenderer::cleanupGraphicsPipelines(VkDevice logicalDevice)
	{
		for(auto& pipeline : mPipelines)
		{
			pipeline.destroy(logicalDevice);
		}
	}

	void VulkanRenderer::createCommandPools()
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = mGraphicsQueueFamilyId;	//Queue family type that buffers trom this command pool will use

		//Create a Graphics Queue Family Command Pool
		VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &mGraphicsCommandPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a graphics command Pool!");
		}

		poolInfo.queueFamilyIndex = mTransferQueueFamilyId;

		result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &mTransferCommandPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a transfer command Pool!");
		}
	}

	void VulkanRenderer::createCommandBuffers()
	{
		mGraphicsCommandBuffers.resize(mFrameBuffers.size());
		mTransferCommandBuffers.resize(mFrameBuffers.size());
		for(int i = 0; i < mFrameBuffers.size(); i++)
		{
			mGraphicsCommandBuffers[i].create(mGraphicsCommandPool, mainDevice.logicalDevice);
			mTransferCommandBuffers[i].create(mTransferCommandPool, mainDevice.logicalDevice);
		}
	}

	void VulkanRenderer::createUniformBuffers()
	{
		//ViewProjection buffer size will be size of all three variables (will offset to access)
		VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

		//ModelMatrix buffer size
		//VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

		//One uniform buffer for each image (and by extension, command buffer)
		vpUniformBuffer.resize(mSwapChain.mSwapChainImages.size());
		vpUniformBufferMemory.resize(mSwapChain.mSwapChainImages.size());

		//modelDUniformBuffer.resize(swapChainImages.size());
		//modelDUniformBufferMemory.resize(swapChainImages.size());

		//Create uniform buffers
		for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
		{
			createBuffer(mainDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);
			/*createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);*/
		}
	}

	void VulkanRenderer::allocateUniformDescriptorSets()
	{
		mUniformDescriptorSets.resize(mSwapChain.mSwapChainImages.size());
		for(uint32_t i = 0; i < mUniformDescriptorSets.size(); i++)
		{
			mUniformDescriptorSets[i].allocate(mainDevice.logicalDevice,
				mUniformDescriptorPool.mDescriptorPool,
				mUniformDescriptorSetLayout.mDescriptorSetLayout);
			mUniformDescriptorSets[i].update(
				mainDevice.logicalDevice,
				vpUniformBuffer[i],
				sizeof(UboViewProjection));
		}
	}

	void VulkanRenderer::allocateInputDescriptorSets()
	{
		//Resize descriptor set array for each swap chain image
		mInputDescriptorSets.resize(mSwapChain.mSwapChainImages.size());
		for(uint32_t i = 0; i < mInputDescriptorSets.size(); i++)
		{
			mInputDescriptorSets[i].allocate(mainDevice.logicalDevice,
			mInputDescriptorPool.mDescriptorPool,
			mInputDescriptorSetLayout.mDescriptorSetLayout);
			mInputDescriptorSets[i].update(
				mainDevice.logicalDevice,
				{
					mFrameBuffers[i].mColorAttachments[0].mImageView,
					mFrameBuffers[i].mDepthAttachment.mImageView,
				},
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				//We don't need a sampler, because we will read it in other way in shader
				VK_NULL_HANDLE);
		}
	}

	void VulkanRenderer::createUI()
	{
		createUIDescriptorPool();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsLight();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
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
		ImGui_ImplVulkan_Init(&init_info);

		io.Fonts->AddFontFromFileTTF("Fonts/Cousine-Regular.ttf", 18);
	}

	void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex, const Camera& camera)
	{
		//Copy VP data
		void* data;
		UboViewProjection vp;
		vp.view = camera.mView;
		vp.projection = camera.mProjection;
		vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
		memcpy(data, &vp, sizeof(UboViewProjection));
		vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);

		//Copy ModelMatrix data
		/*for (size_t i = 0; i < meshList.size(); i++)
		{
			ModelMatrix* thisModel = (ModelMatrix*)((uint64_t)modetTransferSpace + i * modelUniformAlignment);
			*thisModel = meshList[i].getModelMatrix();
		}
		//Map the list of model data
		vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
		memcpy(data, modetTransferSpace, modelUniformAlignment * meshList.size());
		vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);*/
	}

	void VulkanRenderer::onRenderMesh(uint32_t imageIndex, VkCommandBuffer commandBuffer,
		const MeshModel& model, const Mesh& mesh,
		const Camera& camera, const Light& light)
	{
		auto vertexBufferId = mMeshToVertexBufferMap[mesh.getId()];
		auto indexBufferId = mMeshToIndexBufferMap[mesh.getId()];
		if(mBufferManager.isBufferAvailable(vertexBufferId) &&
			mBufferManager.isBufferAvailable(indexBufferId))
		{
			const auto& material = mMaterials[mesh.getMaterialId()];
			const auto& shader = mShaders[material.mShaderId];
			const auto& pipeline = mPipelines[shader.mPipelineId];
			bindPipeline(imageIndex, pipeline.mPipeline);

			const glm::mat4& modelMatrix = model.getModelMatrix();
			//"Push" constants to given shader stage directly (no buffer)
			vkCmdPushConstants(
				commandBuffer,
				pipeline.mPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(glm::mat4),	//Size of data being pushed
				&modelMatrix);	//Actual data being pushed (can be array)

			float sn = static_cast<float>(std::sin(Timer::getInstance().getTime()) * 0.5 + 0.5);
			float xSize = mModelMx.x - mModelMn.x;
			float x = sn * xSize + mModelMn.x;
			x = 0.0f;
			//std::cout << x << std::endl;

			glm::mat3 normalMatrix(modelMatrix);
			normalMatrix = glm::transpose(glm::inverse(normalMatrix));
			Lighting lighting;
			lighting.normalMatrix = glm::mat4(normalMatrix);
			lighting.cameraEye = glm::vec4(-camera.getEye(), 0.0);
			lighting.lightPos = glm::vec4(light.mPosition, material.mShininess);
			lighting.lightColor = glm::vec4(light.mColor, 1.0f);
			//std::cout << "mat3 size: " << sizeof(glm::mat3) << std::endl;
			//std::cout << "Lighting.normalMatrix: " << offsetof(Lighting, normalMatrix) << std::endl;
			//std::cout << "Lighting.cameraEye: " << offsetof(Lighting, cameraEye) << std::endl;
			//std::cout << "Lighting.lightPos: " << offsetof(Lighting, lightPos) << std::endl;
			vkCmdPushConstants(
				commandBuffer,
				pipeline.mPipelineLayout,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				sizeof(glm::mat4),
				sizeof(Lighting),
				&lighting);

			
			const auto& vertexBuffer = mBufferManager.getBuffer(vertexBufferId);
			VkBuffer vertexBuffers[] = { vertexBuffer.mBuffer };	//Buffers to bind
			VkDeviceSize offsets[] = { 0 };		//Offsets into buffers being bound
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);	//Command to bind vertex buffer before drawing with them

			const auto& indexBuffer = mBufferManager.getBuffer(indexBufferId);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer.mBuffer,
				0, VK_INDEX_TYPE_UINT32);	//Command to bind index buffer before drawing with them

			//Dynamic offset amount
			//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

			//Bind all textures of material
			std::vector<VkDescriptorSet> descriptorSets;
			descriptorSets.push_back(mUniformDescriptorSets[imageIndex].mDescriptorSet);
			for(const auto textureId : material.mTextureIds)
			{
				const VulkanDescriptorSet& descriptorSet =
					mTextureManager.getDescriptorSet(mainDevice, mTransferQueueFamilyId,
						mGraphicsQueueFamilyId, mGraphicsQueue, mGraphicsCommandPool, textureId.second);
				descriptorSets.push_back(descriptorSet.mDescriptorSet);
			}

			//are all textures available
			if(material.mTextureIds.size() != descriptorSets.size())
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipeline.mPipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()),
					descriptorSets.data(), 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.getIndexCount()), 1, 0, 0, 0);
			}
		}
	}

	void VulkanRenderer::renderScene(uint32_t imageIndex, const Camera& camera, const Light& light)
	{
		VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[imageIndex].mCommandBuffer;
		for (size_t j = 0; j < mMeshModels.size(); j++)
		{
			const MeshModel& model = mMeshModels[j];

			for (size_t k = 0; k < model.getMeshCount(); k++)
			{
				const Mesh& mesh = model.getMesh(k);
				onRenderMesh(imageIndex, commandBuffer, model, mesh, camera, light);
			}
		}
	}

	void VulkanRenderer::renderTexturedRect(uint32_t imageIndex, VkPipelineLayout pipelineLayout)
	{
		VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[imageIndex].mCommandBuffer;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			0, 1, &mInputDescriptorSets[imageIndex].mDescriptorSet, 0, nullptr);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

	void VulkanRenderer::renderSubPass(uint32_t imageIndex, uint32_t subPassIndex, const Camera& camera,
		const Light& light)
	{
		setViewport(imageIndex);
        setScissor(imageIndex);

        switch(subPassIndex)
        {
        case 0:
            {
                renderScene(imageIndex, camera, light);
            }
            break;
        case 1:
            {
				const auto& fogShader = mShaders[mFogShaderId];
				const auto& fogPipeline = mPipelines[fogShader.mPipelineId];
                bindPipeline(imageIndex, fogPipeline.mPipeline);
                glm::vec2 nearFar(camera.mNear, camera.mFar);
                vkCmdPushConstants(
                    mGraphicsCommandBuffers[imageIndex].mCommandBuffer,
                    fogPipeline.mPipelineLayout,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(glm::vec2),
                    &nearFar);
                renderTexturedRect(imageIndex, fogPipeline.mPipelineLayout);
				drawUI(imageIndex);
            }
            break;
        }
	}

	void VulkanRenderer::loadShader(const std::string& shaderFileName)
	{
		Shader shader;
		shader.mId = static_cast<uint32_t>(mShaders.size());
		shader.mVertexShader.create(mainDevice.logicalDevice, "Shaders/" + shaderFileName + ".vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shader.mFragmentShader.create(mainDevice.logicalDevice, "Shaders/" + shaderFileName + ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		mShaders.push_back(shader);
	}

	void VulkanRenderer::loadUsedShaders()
	{
		mModelMatrixPCR.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//Shader stage push constant will go to
		mModelMatrixPCR.offset = 0;
		mModelMatrixPCR.size = sizeof(glm::mat4);

        mLightingPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mLightingPCR.offset = sizeof(glm::mat4);
		mLightingPCR.size = sizeof(Lighting);

        mNearFarPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mNearFarPCR.offset = 0;
		mNearFarPCR.size = sizeof(glm::vec2);
		
		//Force to load fog shader
		mFogShaderId = getIndexOf(mShaderFileNames, std::string("fog"));
		if(mFogShaderId == -1)
		{
			mFogShaderId = static_cast<int>(mShaderFileNames.size());
			mShaderFileNames.push_back("fog");
		}

		for(const auto& shaderFileName : mShaderFileNames)
		{
			mShaderMetaData.push_back(getShaderMetaData(shaderFileName));
			loadShader(shaderFileName);
		}
	}

	void VulkanRenderer::bindPipeline(uint32_t imageIndex, VkPipeline pipeline)
	{
		vkCmdBindPipeline(
				mGraphicsCommandBuffers[imageIndex].mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline);
	}

	void VulkanRenderer::recordCommands(uint32_t imageIndex, const Camera& camera,
		const Light& light)
	{
		mGraphicsCommandBuffers[imageIndex].begin();
		VkCommandBuffer commandBuffer = mGraphicsCommandBuffers[imageIndex].mCommandBuffer;
		mRenderPass.begin(mFrameBuffers[imageIndex].mFrameBuffer, mSwapChain.mSwapChainExtent,
			commandBuffer, mClearColor);

		for(int32_t i = 0; i < mSubPassesCount; i++)
		{
			renderSubPass(imageIndex, i, camera, light);

			if(i < mSubPassesCount - 1)
			{
				vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			}
		}	

		mRenderPass.end(commandBuffer);
		mGraphicsCommandBuffers[imageIndex].end();
	}

	void VulkanRenderer::getPhysicalDevice()
	{
		//Enumerate physical devices the VkInstance can access
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		//If no devices available, then Vulkan is not supported!
		if (deviceCount == 0)
		{
			throw std::runtime_error("Can't find GPU that supports Vulkan Instance!");
		}
		//Get list of physical devices
		std::vector<VkPhysicalDevice> deviceList(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

		//Just pick first device
		for (const auto& device : deviceList)
		{
			mQueueFamilies = getQueueFamilies(device, surface);
			for(const auto& queueFamily : mQueueFamilies)
			{
				if(queueFamily.mHasGraphicsSupport && queueFamily.mHasPresentationSupport &&
					queueFamily.mHasTransferSupport && checkDeviceSuitable(device))
				{
					mainDevice.physicalDevice = device;
					break;
				}
			}
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

	void VulkanRenderer::recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(mainDevice.logicalDevice);

		mSwapChain.destroy(mainDevice.logicalDevice);
		cleanupSwapChainFrameBuffers();
		cleanupInputDescriptorPool();
		cleanupInputDescriptorSetLayout();
		cleanupSwapchainImagesSemaphores();

		mSwapChain.create(window, mainDevice, mGraphicsQueueFamilyId,
			mPresentationQueueFamilyId, surface);
		createSwapChainFrameBuffers();
		createInputDescriptorPool();
		createInputDescriptorSetLayout();
		allocateInputDescriptorSets();

		createSwapchainImagesSemaphores();
	}

	bool VulkanRenderer::checkInstanceExtentionsSupport(std::vector<const char*>* checkExtentions)
	{
		//Need to get number of extentions to create array of correct size to hold extentions
		uint32_t extentionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extentionsCount, nullptr);

		//Create a list of VkExtentionProperties using count
		std::vector<VkExtensionProperties> extentions(extentionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extentionsCount, extentions.data());

		//Check if given extenxtions are in list of available extentions
		for (const auto& checkExtention : *checkExtentions)
		{
			bool hasExtention = false;
			for (const auto& extention : extentions)
			{
				if (strcmp(checkExtention, extention.extensionName) == 0)
				{
					hasExtention = true;
					break;
				}
			}

			if (!hasExtention)
			{
				return false;
			}
		}

		return true;
	}

	bool VulkanRenderer::checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

	bool VulkanRenderer::checkDeviceExtentionSupport(VkPhysicalDevice device)
	{
		//Get device extention count
		uint32_t extentionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extentionCount, nullptr);

		//If no extensions found, return failure
		if (extentionCount == 0)
		{
			return false;
		}

		//Populate list of extensions
		std::vector<VkExtensionProperties> extensions(extentionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extentionCount, extensions.data());

		for (const auto& deviceExtension : deviceExtentions)
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

	bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
	{
		/*
		//Information about the device itself (ID, name, type, Vendor, etc)
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);*/

		//Information about what device can do (geo shader, tess shaders, wide lines, etc.)
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		bool extensionSupported = checkDeviceExtentionSupport(device);

		bool swapChainValid = false;
		if (extensionSupported)
		{
			SwapChainDetails swapChainDetails = mSwapChain.getSwapChainDetails(device, surface);
			swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
		}

		return extensionSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
	}

	void VulkanRenderer::createSwapchainImagesSemaphores()
	{
		imageAvailable.resize(MAX_FRAME_DRAWS);
		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create swapchain-image Semaphore!");
			}
		}
	}

	void VulkanRenderer::createRenderFinishedSemaphores()
	{
		renderFinished.resize(MAX_FRAME_DRAWS);
		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create render-finished Semaphore!");
			}
		}
	}

	void VulkanRenderer::createDrawFences()
	{
		drawFences.resize(MAX_FRAME_DRAWS);
		//Fence creation information
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			if (vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create a Semaphore and/ordraw Fence!");
			}
		}
	}

	void VulkanRenderer::createTransferSynchronisation()
	{
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreInfo, nullptr, &mTransferCompleteSemaphore);

		VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(mainDevice.logicalDevice, &fenceInfo, nullptr, &mTransferFence);
	}

	void VulkanRenderer::createSynchronisation()
	{
		createSwapchainImagesSemaphores();
		createRenderFinishedSemaphores();
		createDrawFences();
		createTransferSynchronisation();
	}

    void VulkanRenderer::setViewport(uint32_t imageIndex)
    {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) mSwapChain.mSwapChainExtent.width;
		viewport.height = (float) mSwapChain.mSwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(mGraphicsCommandBuffers[imageIndex].mCommandBuffer, 0, 1, &viewport);
    }

    void VulkanRenderer::setScissor(uint32_t imageIndex)
    {
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = mSwapChain.mSwapChainExtent;
		vkCmdSetScissor(mGraphicsCommandBuffers[imageIndex].mCommandBuffer, 0, 1, &scissor);
    }

	void VulkanRenderer::loadImages()
	{
		//std::cout << "render tid: " << std::this_thread::get_id() << std::endl;
		mStatistics.startMeasure("load images", Timer::getInstance().getTime());
		for(uint32_t i = 0; i < mTextureFileNames.size(); i++)
		{
			const auto& fileName = mTextureFileNames[i];
			//load default texture in main thread
			if(i == 0)
			{
				mTextureManager.loadImage(fileName, i);
				mTextureManager.getDescriptorSet(mainDevice, mTransferQueueFamilyId,
					mGraphicsQueueFamilyId, mGraphicsQueue, mGraphicsCommandPool, i);
			}
			else
			{
				mThreadPool.enqueue
				(
					[this, &fileName, i]
					{
						this->mTextureManager.loadImage(fileName, i);
						if(this->mTextureManager.getImagesCount() == this->mTextureFileNames.size())
						{
							this->mStatistics.stopMeasure("load images", Timer::getInstance().getTime());
							this->mStatistics.print();
						}
					}
				);
			}
		}
	}

	void VulkanRenderer::loadMeshes()
	{
		for(auto& meshModel : mMeshModels)
		{
			for(uint32_t i = 0; i < meshModel.getMeshCount(); i++)
			{
				const auto& mesh = meshModel.getMesh(i);
				uint32_t meshId = mesh.getId();
				const void* vertexData = mesh.getVertexData();
				const void* indexData = mesh.getIndexData();
				uint32_t vertexBufferSize = mesh.getVertexCount() * sizeof(Vertex);
				uint32_t indexBufferSize = mesh.getIndexCount() * sizeof(uint32_t);
				mMeshToVertexBufferMap[meshId] = static_cast<uint32_t>(mBufferManager.mBuffers.size());
				mBufferManager.createBuffer(
					mainDevice, mTransferQueue, mTransferCommandPool,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexData,
					vertexBufferSize);
				mMeshToIndexBufferMap[meshId] = static_cast<uint32_t>(mBufferManager.mBuffers.size());
				mBufferManager.createBuffer(
					mainDevice, mTransferQueue, mTransferCommandPool,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexData,
					indexBufferSize);
			}
		}
	}
	
	ShaderMetaData VulkanRenderer::getShaderMetaData(const std::string& shaderFileName) const
	{
		ShaderMetaData shaderMetaData;

		if(shaderFileName == "pbr")
		{
			shaderMetaData.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			shaderMetaData.mDescriptorSetLayouts =
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
			shaderMetaData.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			shaderMetaData.mDepthTestEnabled = true;
			shaderMetaData.mVertexSize = sizeof(Vertex);
			shaderMetaData.mSubPassIndex = 0;
		}
		else if(shaderFileName == "normalMap")
		{
			shaderMetaData.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			shaderMetaData.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout,
				//Color texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
				//Normals texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
			};
			shaderMetaData.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			shaderMetaData.mDepthTestEnabled = true;
			shaderMetaData.mVertexSize = sizeof(Vertex);
			shaderMetaData.mSubPassIndex = 0;
		}
		else if(shaderFileName == "textured")
		{
			shaderMetaData.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			shaderMetaData.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout,
				//Color texture for subpass 0
				mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
			};
			shaderMetaData.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			shaderMetaData.mDepthTestEnabled = true;
			shaderMetaData.mVertexSize = sizeof(Vertex);
			shaderMetaData.mSubPassIndex = 0;
		}
		else if(shaderFileName == "dhm")
		{
			shaderMetaData.mVertexAttributes =
			{
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
				{VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
			};
			shaderMetaData.mDescriptorSetLayouts =
			{
				//All inputs used in render pass
				//Uniforms (model matrix) in subpass 0
				mUniformDescriptorSetLayout.mDescriptorSetLayout
			};
			shaderMetaData.mPushConstantRanges = {mModelMatrixPCR, mLightingPCR};
			shaderMetaData.mDepthTestEnabled = true;
			//shaderMetaData.mPolygonMode = VK_POLYGON_MODE_LINE;
			shaderMetaData.mVertexSize = sizeof(Vertex);
			shaderMetaData.mSubPassIndex = 0;
		}
		else if(shaderFileName == "fog")
		{
			shaderMetaData.mDescriptorSetLayouts = {mInputDescriptorSetLayout.mDescriptorSetLayout};
			shaderMetaData.mPushConstantRanges = {mNearFarPCR};
			shaderMetaData.mDepthTestEnabled = false;
			shaderMetaData.mVertexSize = 0;
			shaderMetaData.mSubPassIndex = 1;
		}
		
		return shaderMetaData;
	}

	void VulkanRenderer::addMaterial(Material& material)
	{
		std::string shaderFileName;
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
			shaderFileName = "colored";
			//shaderFileName = "dhm";
		}

		if(!shaderFileName.empty())
		{
			int foundShaderId = getIndexOf(mShaderFileNames, shaderFileName);
			if(foundShaderId == -1)
			{
				material.mShaderId = static_cast<uint32_t>(mShaderFileNames.size());
				mShaderFileNames.push_back(shaderFileName);
			}
			else
			{
				material.mShaderId = foundShaderId;
			}
		}
		mMaterials.push_back(material);
	}

	int VulkanRenderer::createMeshModel(std::string modelFile,
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
			throw::std::runtime_error("Failed to load model! (" + modelFile + ")");
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
				material.mShininess = 16.0f;
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
						//Get file name
						auto idx = std::string(path.data).rfind("\\");
						if(idx == -1)
						{
							idx = std::string(path.data).rfind("/");
						}
						std::string textureFileName = std::string(path.data).substr(idx + 1);
						const auto foundTexId = getIndexOf(mTextureFileNames, textureFileName);
						if(foundTexId == -1)
						{
							material.mTextureIds[textureType] =
								static_cast<uint32_t>(mTextureFileNames.size());
							mTextureFileNames.push_back(textureFileName);
						}
						else
						{
							material.mTextureIds[textureType] = foundTexId;
						}
					}
				}
			}

			addMaterial(material);
		}

		//Load all meshes
		std::vector<Mesh> modelMeshes = MeshModel::loadNode(
			scene->mRootNode, scene, mModelMn, mModelMx, materialsOffset);

		MeshModel meshModel = MeshModel(modelMeshes);
		mMeshModels.push_back(meshModel);

		return static_cast<int>(mMeshModels.size()) - 1;
	}
}