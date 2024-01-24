#include "Renderer/VulkanRenderer.hpp"

#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanShader.hpp"
#include "config.hpp"

#include <array>
#include <limits>
#include <stdexcept>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace fre
{
	VulkanRenderer::VulkanRenderer()
	{
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
			mSwapChain.create(window, mainDevice, surface);
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
			createPushConstantRange();

			createGraphicsPipelines();
			createCommandPool();
			createCommandBuffers();
			createSynchronisation();

			createCamera();
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

		//_aligned_free(modetTransferSpace);

		for (size_t i = 0; i < mMeshModels.size(); i++)
		{
			mMeshModels[i].destroyMeshModel(mainDevice.logicalDevice);
		}

		cleanupInputDescriptorPool();
		mTextureManager.destroy(mainDevice.logicalDevice);

		mSwapChain.destroy(mainDevice.logicalDevice);
		cleanupSwapChainFrameBuffers();

		mUniformDescriptorPool.destroy(mainDevice.logicalDevice);
		mUniformDescriptorSetLayout.destroy(mainDevice.logicalDevice);

		for (size_t i = 0; i < mSwapChain.mSwapChainImages.size(); i++)
		{
			vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);
			
			//vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
			//vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
		}

		cleanupSwapchainImagesSemaphores();
		cleanupRenderFinishedSemaphors();
		cleanupDrawFences();

		vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
		
		cleanupGraphicsPipelines(mainDevice.logicalDevice);

		mRenderPass.destroy(mainDevice.logicalDevice);
		
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(mainDevice.logicalDevice, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void VulkanRenderer::updateModel(int modelId, glm::mat4 newModelMatrix)
	{
		if (modelId < mMeshModels.size())
		{
			mMeshModels[modelId].setModelMatrix(newModelMatrix);
		}
	}

	void VulkanRenderer::draw()
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

			recordCommands(imageIndex);

			updateUniformBuffers(imageIndex);

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
			VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex].mCommandBuffer;
			submitInfo.pCommandBuffers = &commandBuffer;	//Command buffer to submit
			submitInfo.signalSemaphoreCount = 1;	//Number of semaphores to signal
			submitInfo.pSignalSemaphores = &renderFinished[currentFrame];	//Semaphore to signal wen command buffer finishes

			result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
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

			result = vkQueuePresentKHR(presentationQueue, &presentInfo);

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

	void VulkanRenderer::cleanupSwapChainFrameBuffers()
	{
		//destroy frame buffers
		for(auto& fbo : mFrameBuffers)
		{
			fbo.destroy(mainDevice.logicalDevice);
		}
	}

	void VulkanRenderer::cleanupInputDescriptorPool()
	{
		mInputDescriptorPool.destroy(mainDevice.logicalDevice);
		mInputDescriptorSetLayout.destroy(mainDevice.logicalDevice);
	}

    void VulkanRenderer::cleanupSwapchainImagesSemaphores()
    {
		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
		}
    }

    void VulkanRenderer::cleanupRenderFinishedSemaphors()
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

    void VulkanRenderer::setFramebufferResized(bool resized)
    {
		framebufferResized = resized;
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
		//Get the queue family indices for the chosen Physical Device
		QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice, surface);

		//Vector for queue creation information, and set for family indices
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };
		
		//Queue the logical device needs to create and info to do so
		for(int queueFamilyIndex : queueFamilyIndices)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamilyIndex;			//The index of the family to create queue from
			queueCreateInfo.queueCount = 1;									//Number of queues to create
			float priority = 1.0f;
			queueCreateInfo.pQueuePriorities = &priority;					//Vulkan needs to know how to handle multiple queues, so decide priority (1 - highest priority)

			queueCreateInfos.push_back(queueCreateInfo);
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
		vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
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
			static_cast<uint32_t>(mSwapChain.mSwapChainImages.size()),
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER});
	}

	void VulkanRenderer::createInputDescriptorPool()
	{
		//pool for color and depth attachments
		mInputDescriptorPool.create(
			mainDevice.logicalDevice,
			MAX_OBJECTS,
			{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT});
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

	void VulkanRenderer::createPushConstantRange()
	{
		//Define push constant value (no "create" need!)
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	//Shader stage push constant will go to
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);

		//Define push constant value (no "create" need!)
		pushConstantRangeNearFar.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		pushConstantRangeNearFar.offset = 0;
		pushConstantRangeNearFar.size = sizeof(glm::vec2);
	}

	void VulkanRenderer::createGraphicsPipelines()
	{
		
	}

	void VulkanRenderer::cleanupGraphicsPipelines(VkDevice logicalDevice)
	{
		
	}

	void VulkanRenderer::createCommandPool()
	{
		//Get indices of queue families from device;
		QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	//Queue family type that buffers trom this command pool will use

		//Create a Graphics Queue Family Command Pool
		VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Command Pool!");
		}
	}

	void VulkanRenderer::createCommandBuffers()
	{
		mCommandBuffers.resize(mFrameBuffers.size());
		for(auto& cb : mCommandBuffers)
		{
			cb.create(graphicsCommandPool, mainDevice.logicalDevice);
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

	void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
	{
		//Copy VP data
		void* data;
		UboViewProjection vp;
		vp.view = mCamera.mView;
		vp.projection = mCamera.mProjection;
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

	void VulkanRenderer::onRenderModel(VkCommandBuffer commandBuffer,
		VkPipelineLayout pipelineLayout, const MeshModel& meshModel)
	{
		
	}

	void VulkanRenderer::renderScene(uint32_t imageIndex, VkPipelineLayout pipelineLayout)
	{
		VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex].mCommandBuffer;
		for (size_t j = 0; j < mMeshModels.size(); j++)
		{
			const MeshModel& thisModel = mMeshModels[j];
			
			onRenderModel(commandBuffer, pipelineLayout, thisModel);

			for (size_t k = 0; k < thisModel.getMeshCount(); k++)
			{
				VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer()};	//Buffers to bind
				VkDeviceSize offsets[] = { 0 };		//Offsets into buffers being bound
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);	//Command to bind vertex buffer before drawing with them

				vkCmdBindIndexBuffer(commandBuffer, thisModel.getMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);	//Command to bind index buffer before drawing with them

				//Dynamic offset amount
				//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

				//Bind all textures of material
				std::vector<VkDescriptorSet> descriptorSets;
				descriptorSets.push_back(mUniformDescriptorSets[imageIndex].mDescriptorSet);
				const auto& material = mMaterials[thisModel.getMesh(k)->getMaterialId()];
				for(const auto textureId : material.mTextureIds)
				{
					descriptorSets.push_back(
						mTextureManager.mSamplerDescriptorSets[textureId.second].mDescriptorSet
					);
				}
				/*const auto diffuseIt = material.mTextureIds.find(aiTextureType_DIFFUSE);
				const auto textureId = diffuseIt != material.mTextureIds.end() ? diffuseIt->second : 0;
				std::array<VkDescriptorSet, 2> descriptorSetGroup =
					{
						mUniformDescriptorSets[imageIndex].mDescriptorSet,
						mTextureManager.mSamplerDescriptorSets[textureId].mDescriptorSet
					};*/

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()),
					descriptorSets.data(), 0, nullptr);

				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(thisModel.getMesh(k)->getIndexCount()), 1, 0, 0, 0);
			}
		}
	}

	void VulkanRenderer::renderTexturedRect(uint32_t imageIndex, VkPipelineLayout pipelineLayout)
	{
		VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex].mCommandBuffer;

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			0, 1, &mInputDescriptorSets[imageIndex].mDescriptorSet, 0, nullptr);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	}

	void VulkanRenderer::renderSubPass(uint32_t imageIndex, uint32_t subPassIndex)
	{
		
	}

	void VulkanRenderer::bindPipeline(uint32_t imageIndex, VkPipeline pipeline)
	{
		vkCmdBindPipeline(
				mCommandBuffers[imageIndex].mCommandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline);
	}

	void VulkanRenderer::recordCommands(uint32_t imageIndex)
	{
		mCommandBuffers[imageIndex].begin();
		VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex].mCommandBuffer;
		mRenderPass.begin(mFrameBuffers[imageIndex].mFrameBuffer, mSwapChain.mSwapChainExtent, commandBuffer);

		for(int32_t i = 0; i < mSubPassesCount; i++)
		{
			renderSubPass(imageIndex, i);

			if(i < mSubPassesCount - 1)
			{
				vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			}
		}	

		mRenderPass.end(commandBuffer);
		mCommandBuffers[imageIndex].end();
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
			if (checkDeviceSuitable(device))
			{
				mainDevice.physicalDevice = device;
				break;
			}
		}

		//Get properties of device
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

		//minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
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
		cleanupSwapchainImagesSemaphores();

		mSwapChain.create(window, mainDevice, surface);
		createSwapChainFrameBuffers();
		createInputDescriptorSetLayout();
		createInputDescriptorPool();
		allocateInputDescriptorSets();

		createSwapchainImagesSemaphores();

		createCamera();
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
		
		QueueFamilyIndices indices = getQueueFamilies(device, surface);

		bool extensionSupported = checkDeviceExtentionSupport(device);

		bool swapChainValid = false;
		if (extensionSupported)
		{
			SwapChainDetails swapChainDetails = mSwapChain.getSwapChainDetails(device, surface);
			swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
		}

		return indices.isValid() && extensionSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
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

	void VulkanRenderer::createCamera()
	{
		float aspectRatio = (float)mSwapChain.mSwapChainExtent.width /
			(float)mSwapChain.mSwapChainExtent.height;
		mCamera.setPerspectiveProjection(45.0f, aspectRatio, 0.1f, 100.0f);
		mCamera.set(glm::vec3(30.0f, 30.0f, 0.0f), glm::vec3(0.0f, 5.0f, 0.0f));
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

	void VulkanRenderer::createSynchronisation()
	{
		createSwapchainImagesSemaphores();
		createRenderFinishedSemaphores();
		createDrawFences();
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
		vkCmdSetViewport(mCommandBuffers[imageIndex].mCommandBuffer, 0, 1, &viewport);
    }

    void VulkanRenderer::setScissor(uint32_t imageIndex)
    {
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = mSwapChain.mSwapChainExtent;
		vkCmdSetScissor(mCommandBuffers[imageIndex].mCommandBuffer, 0, 1, &scissor);
    }

	int VulkanRenderer::createMeshModel(std::string modelFile)
	{
		//Import model scene
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
		if (!scene)
		{
			throw::std::runtime_error("Failed to load model! (" + modelFile + ")");
		}

		std::vector<std::map<aiTextureType, std::string>> materials =
			MeshModel::loadMaterials(scene, static_cast<aiTextureType>(
				aiTextureType_DIFFUSE | aiTextureType_NORMALS));
		//To prevent texture duplicates
		//1. create unique list of texture file paths
		std::set<std::string> uniqueTextureFilePathsTmp;
		for(auto& material : materials)
		{
			for(auto& textureFilePath : material)
			{
				uniqueTextureFilePathsTmp.insert(textureFilePath.second);
			}
		}
		std::vector<std::string> uniqueTextureFilePaths(
			uniqueTextureFilePathsTmp.begin(), uniqueTextureFilePathsTmp.end());

		//2. load all unique textures
		for(auto& textureFilePath : uniqueTextureFilePaths)
		{
			mTextureManager.createTexture(mainDevice, graphicsQueue,
				graphicsCommandPool, textureFilePath);
		}
		//3. create materials that refer unique textures
		for(auto& material : materials)
		{
			mMaterials.push_back(Material());
			for(auto& textureFilePath : material)
			{
				//find index of texture inside unique list
				auto found = std::find(
					uniqueTextureFilePaths.begin(),
					uniqueTextureFilePaths.end(),
					textureFilePath.second);
				auto textureIndex = found - uniqueTextureFilePaths.begin();
				mMaterials.back().mTextureIds[textureFilePath.first] = static_cast<uint32_t>(textureIndex);
			}
		}
 
		//Load all meshes
		std::vector<Mesh> modelMeshes = MeshModel::loadNode(
			mainDevice, graphicsQueue, graphicsCommandPool,
			scene->mRootNode, scene);

		MeshModel meshModel = MeshModel(modelMeshes);
		mMeshModels.push_back(meshModel);

		return mMeshModels.size() - 1;
	}
}