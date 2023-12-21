#include "Renderer/VulkanRenderer.hpp"

#include <array>

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

	int VulkanRenderer::init(GLFWwindow* newWindow)
	{
		window = newWindow;

		try {
			createInstance();
			createSurface();
			getPhysicalDevice();
			createLogicalDevice();
			createSwapChain();
			createRenderPass();
			createDescriptorSetLayout();
			createPushConstantRange();
			createGraphicsPipeline();
			createColourBufferImage();
			createDepthBufferImage();
			createFrameBuffers();
			createCommandPool();

			createCommandBuffers();
			createTextureSampler();
			//allocateDynamicBufferTransferSpace();
			createUniformBuffers();
			createDescriptorPool();
			createDescriptorSets();
			createInputDescriptorSets();
			createSynchronisation();

			uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
			uboViewProjection.view = glm::lookAt(glm::vec3(30.0f, 30.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			//In Vulkan Up direction points down
			uboViewProjection.projection[1][1] *= -1.0f;
			createTexture("test.jpg");
		}
		catch (std::runtime_error& e)
		{
			printf("ERROR: %s\n", e.what());
			return EXIT_FAILURE;
		}


		return 0;
	}

	void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel)
	{
		if (modelId >= modelList.size()) return;
		modelList[modelId].setModel(newModel);
	}

	void VulkanRenderer::draw()
	{
		uint32_t imageIndex;
		//Get index of next image to be drawn to, and sibnal semaphore when ready to be drawn to
		vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

		// -- GET NEXT IMAGE--
		//Wait for given fence to signal (open) from last draw before continuing
		vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint32_t>::max());
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
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];	//Command buffer to submit
		submitInfo.signalSemaphoreCount = 1;	//Number of semaphores to signal
		submitInfo.pSignalSemaphores = &renderFinished[currentFrame];	//Semaphore to signal wen command buffer finishes

		VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
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
		presentInfo.pSwapchains = &swapchain;	//Swapchains to present images to
		presentInfo.pImageIndices = &imageIndex;	//Indices of images in swapchain to present

		result = vkQueuePresentKHR(presentationQueue, &presentInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present Image!");
		}

		//Get next frame
		currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
	}

	void VulkanRenderer::cleanup()
	{
		//Wait until no actions being run on device before destroying
		vkDeviceWaitIdle(mainDevice.logicalDevice);

		//_aligned_free(modetTransferSpace);

		for (size_t i = 0; i < modelList.size(); i++)
		{
			modelList[i].destroyMeshModel();
		}

		vkDestroyDescriptorPool(mainDevice.logicalDevice, inputDescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, inputSetLayout, nullptr);

		vkDestroyDescriptorPool(mainDevice.logicalDevice, samplerDescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, samplerSetLayout, nullptr);

		vkDestroySampler(mainDevice.logicalDevice, textureSampler, nullptr);

		for (size_t i = 0; i < textureImages.size(); i++)
		{
			vkDestroyImageView(mainDevice.logicalDevice, textureImageViews[i], nullptr);
			vkDestroyImage(mainDevice.logicalDevice, textureImages[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, textureImageMemory[i], nullptr);
		}

		for (size_t i = 0; i < colourBufferImage.size(); i++)
		{
			vkDestroyImageView(mainDevice.logicalDevice, colourBufferImageView[i], nullptr);
			vkDestroyImage(mainDevice.logicalDevice, colourBufferImage[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, colourBufferImageMemory[i], nullptr);
		}

		for (size_t i = 0; i < depthBufferImage.size(); i++)
		{
			vkDestroyImageView(mainDevice.logicalDevice, depthBufferImageView[i], nullptr);
			vkDestroyImage(mainDevice.logicalDevice, depthBufferImage[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, depthBufferImageMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
			vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);
			
			//vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
			//vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
		}

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
			vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
			vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
		}
		vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
		for (auto framebuffer : swapChainFrameBuffers)
		{
			vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
		}
		vkDestroyPipeline(mainDevice.logicalDevice, secondPipeline, nullptr);
		vkDestroyPipelineLayout(mainDevice.logicalDevice, secondPipelineLayout, nullptr);
		vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
		vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);
		for (auto image : swapChainImages)
		{
			vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
		}
		vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(mainDevice.logicalDevice, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void VulkanRenderer::createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		//Information about application itself
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom version of the application
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);	//Custom engine version
		appInfo.apiVersion = VK_API_VERSION_1_0;			//Vulkan version

		//Creation information for VkInstance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		//Create list of extentions
		std::vector<const char*> instanceExtentions = std::vector<const char*>();
		//Setup extentions instance will use
		uint32_t glfwExtentionsCount = 0;
		const char** glfwExtentions;			//Extentions are passed as array of cstrings
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
		QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

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

	void VulkanRenderer::createSurface()
	{
		//Create surface (creates a surface create info struct, runs the create surface function, returns result)
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a surface!");
		}
	}

	void VulkanRenderer::createSwapChain()
	{
		//Get swap chain details so we can pick best settings
		SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

		//Find optimal surface values for our swap chain

		//1. CHOOSE BEST SURFACE FORMAT
		VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
		//2. CHOOSET BEST PRESENTATION MODE
		VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
		//3. CHOOSE SWAP CHAIN IMAGE RESOLUTION
		VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

		//How many images are in swap chain? Get 1 more than the minimum to allow tripple buffering
		uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

		//If there is limit of max images (0 - limitless)
		//Clamp by max
		if (swapChainDetails.surfaceCapabilities.maxImageCount > 0
			&& swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
		{
			imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
		}
		//Creation information for swap chain
		VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = surface;
		swapChainCreateInfo.imageFormat = surfaceFormat.format;
		swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainCreateInfo.presentMode = presentMode;
		swapChainCreateInfo.imageExtent = extent;
		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageArrayLayers = 1;				//Number of layers for each image in chain
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//What attachment images will be used as
		swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;	//Transform to perform on swap chain images
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		//How to handle blending images with external graphics (e. g. other windows)
		swapChainCreateInfo.clipped = VK_TRUE;		//Where to clip parts of image not in view (e.g. ghind another window, off screen, etc.)

		//Get queue family indices
		QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);
		//If Graphics and Presentation families are different, then swap chain must let images be shared between families
		if (indices.graphicsFamily != indices.presentationFamily)
		{
			//Queues to share between
			uint32_t queueFamilyIndices[] =
			{
				(uint32_t)indices.graphicsFamily,
				(uint32_t)indices.presentationFamily
			};
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	//Image share handling
			swapChainCreateInfo.queueFamilyIndexCount = 2;						//Number of queues to share images between
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;		//Array of queues to share between
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		//If old swap chain been destroyed and this one replace it, then ling old one to quickle hand over responsibilities
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		//Create swapchain
		VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a swapchain!");
		}

		//Store for later reference
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		//Get swap chain images
		uint32_t swapChainImageCount;
		vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);
		std::vector<VkImage> images(swapChainImageCount);
		vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data());

		for (VkImage image : images)
		{
			//Store image handle
			SwapChainImage swapChainImage = {};
			swapChainImage.image = image;
			swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

			//Add to swapchain image list
			swapChainImages.push_back(swapChainImage);
		}

	}

	void VulkanRenderer::createRenderPass()
	{
		//Array of subpasses
		std::array<VkSubpassDescription, 2> subpasses{};

		//ATTACHMENTS
		//SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

		//Colour Attachment (Input)
		VkAttachmentDescription colourAttachment = {};
		colourAttachment.format = chooseSupportedFormat(
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	//Before render pass
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Don't care after render pass
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Image data layout after render pass (to change to)

		//Depth attachment (Input)
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = chooseSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//We need to store after rendering to present image on screen
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//Colour Attachment (Input) Reference
		VkAttachmentReference colourAttachmentReference = {};
		colourAttachmentReference.attachment = 1;	//This is attachment index in (std::array<VkImageView, 3> attachments) in createFrameBuffers()
		colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	//Layout before subpass

		//Depth Attachment (Input) Reference
		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 2;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//Layout before subpass

		//Setup subpass 1
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[0].colorAttachmentCount = 1;
		subpasses[0].pColorAttachments = &colourAttachmentReference;
		subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

		//SUBPASS 2 ATTACHMENTS + REFERENCES

		//Swapchain colour attachment
		VkAttachmentDescription swapchainColourAttachment = {};
		swapchainColourAttachment.format = swapChainImageFormat;
		swapchainColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		swapchainColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		swapchainColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	//We need to store after rendering to present image on screen
		swapchainColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		swapchainColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		
		//Framebuffer data will be stored as an image, but images can be given different data layouts
		//to give optimal use for certain operations
		swapchainColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Image data layout before render pass starts
		swapchainColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	//Image data layout after render pass (to change to)

		//Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
		VkAttachmentReference swapchainColourAttachmentReference = {};
		swapchainColourAttachmentReference.attachment = 0;
		swapchainColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//References to attachments that subpass will take input from
		std::array<VkAttachmentReference, 2> inputReferences;
		inputReferences[0].attachment = 1;	//Colour
		inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass
		inputReferences[1].attachment = 2;	//Depth
		inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Layout before subpass

		//Setup subpass 2
		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	//pipeline type subpass is to be bound to
		subpasses[1].colorAttachmentCount = 1;
		subpasses[1].pColorAttachments = &swapchainColourAttachmentReference;
		subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
		subpasses[1].pInputAttachments = inputReferences.data();

		//SUBPASS DEPENDENCIES
		//Need to determine when layout transitions occur using subpass dependencies
		std::array<VkSubpassDependency, 3> subpassDependencies;

		//Convertion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		//Transition must happen after...
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;		//Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		//Pipeline stage
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				//Stage access mask (memory access)
		//But must happen before
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;

		//Subpass 1 layout (color/depth) to Subpass 2 layout (shader read)
		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstSubpass = 1;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependencies[1].dependencyFlags = 0;

		//Convertion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		//Transition must happen after...
		subpassDependencies[2].srcSubpass = 0;
		subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//But must happen before
		subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[2].dependencyFlags = 0;

		std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColourAttachment, colourAttachment, depthAttachment };

		//Create info for render pass
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
		renderPassCreateInfo.pAttachments = renderPassAttachments.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
		renderPassCreateInfo.pSubpasses = subpasses.data();
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassCreateInfo.pDependencies = subpassDependencies.data();

		VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void VulkanRenderer::createDescriptorSetLayout()
	{
		//VP binding info
		VkDescriptorSetLayoutBinding vpLayoutBinding = {};
		vpLayoutBinding.binding = 0;	//Binding point in shader (designated by binding number in shader)
		vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	//Type of descriptor (uniform, dynamic uniform, image sample, etc.)
		vpLayoutBinding.descriptorCount = 1;	//Number of descriptors for binding
		vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	//Shader stage we bind to
		vpLayoutBinding.pImmutableSamplers = nullptr;	//Immutability by specifying the layout

		//Model Binding info
		/*VkDescriptorSetLayoutBinding modelLayoutBinding = {};
		modelLayoutBinding.binding = 1;
		modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelLayoutBinding.descriptorCount = 1;
		modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		modelLayoutBinding.pImmutableSamplers = nullptr;*/

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

		//Create descriptor set layout with given bindings
		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutCreateInfo.pBindings = layoutBindings.data();

		//Create descriptor set layout
		VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Descriptor Set Layout!");
		}

		//CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 0;	//Binding point in shader (designated by binding number in shader)
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	//Type of descriptor (uniform, dynamic uniform, image sampler, etc.)
		samplerLayoutBinding.descriptorCount = 1;	//Number of descriptors for binding
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage we bind to
		samplerLayoutBinding.pImmutableSamplers = nullptr;	//Immutability by specifying the layout

		VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
		textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		textureLayoutCreateInfo.bindingCount = 1;
		textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

		//Create sampler descriptor set layout
		result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr, &samplerSetLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Sampler Descriptor Set Layout!");
		}
		
		//CREATE INPUT ATTACHMENT IMAGE DESCRIPTOR SET LAYOUT
		//Colour input binding
		VkDescriptorSetLayoutBinding colourInputLayoutBinding = {};
		colourInputLayoutBinding.binding = 0;	//Binding point in shader (designated by binding number in shader)
		colourInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;	//Type of descriptor (uniform, dynamic uniform, image sampler, etc.)
		colourInputLayoutBinding.descriptorCount = 1;	//Number of descriptors for binding
		colourInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage we bind to

		VkDescriptorSetLayoutBinding depthInputLayoutBinding = {};
		depthInputLayoutBinding.binding = 1;	//Binding point in shader (designated by binding number in shader)
		depthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;	//Type of descriptor (uniform, dynamic uniform, image sampler, etc.)
		depthInputLayoutBinding.descriptorCount = 1;	//Number of descriptors for binding
		depthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage we bind to

		//Array of input attachment bindings
		std::vector<VkDescriptorSetLayoutBinding> inputBindings = { colourInputLayoutBinding, depthInputLayoutBinding };

		//Create a descriptor set layout for input attachments
		VkDescriptorSetLayoutCreateInfo inputLayoutCreateInfo = {};
		inputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		inputLayoutCreateInfo.bindingCount = static_cast<uint32_t>(inputBindings.size());
		inputLayoutCreateInfo.pBindings = inputBindings.data();

		//Create descriptor set layout
		result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &inputLayoutCreateInfo, nullptr, &inputSetLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Input Descriptor Set Layout!");
		}
	}

	void VulkanRenderer::createPushConstantRange()
	{
		//Define push constant value (no "create" need!)
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	//Shader stage push constant will go to
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(Model);
	}

	void VulkanRenderer::createGraphicsPipeline()
	{
		auto vertexShaderCode = readFile("Shaders/vert.spv");
		auto fragmentShaderCode = readFile("Shaders/frag.spv");

		VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
		
		// -- SHADER STAGE CREATION INFORMATION --
		//Vertex stage create info
		VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
		vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;		//Shader stage name
		vertexShaderCreateInfo.module = vertexShaderModule;				//Shader module to be used by stage
		vertexShaderCreateInfo.pName = "main";							//Entry point in to shader

		VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
		fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;		//Shader stage name
		fragmentShaderCreateInfo.module = fragmentShaderModule;				//Shader module to be used by stage
		fragmentShaderCreateInfo.pName = "main";							//Entry point in to shader

		//Put shader stage creation info in to array
		//Graphics Pipeline creation info requires array of shader stage creates
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

		//How data for the single vertex (position, color, tex coords, normals, etc.) is as whole
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;	//Can bind multiple streams of data, this defines which one
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;	//How to move between data after each vertex (instanced rendering or not, etc.)

		//How the data for an attribute is defined within a vertex
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

		//Position Attribute
		attributeDescriptions[0].binding = 0;	//Which binding the data is at (should be the same as above)
		attributeDescriptions[0].location = 0;	//Location in shader where data will be read from
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	//Data format
		attributeDescriptions[0].offset = offsetof(Vertex, pos);	//Where this attribute is defined in the data for a single vertex

		//Position Attribute
		attributeDescriptions[1].binding = 0;	//Which binding the data is at (should be the same as above)
		attributeDescriptions[1].location = 1;	//Location in shader where data will be read from
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;	//Data format
		attributeDescriptions[1].offset = offsetof(Vertex, col);	//Where this attribute is defined in the data for a single vertex

		//Position Attribute
		attributeDescriptions[2].binding = 0;	//Which binding the data is at (should be the same as above)
		attributeDescriptions[2].location = 2;	//Location in shader where data will be read from
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;	//Data format
		attributeDescriptions[2].offset = offsetof(Vertex, tex);	//Where this attribute is defined in the data for a single vertex

		// -- VERTEX INPUT --
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;		//List of vertex binding descriptions (data spacing/stride info)
		vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();	//List of vertex attribute descriptions (data format and where to bind to/from)

		// -- INPUT ASSEMBLY --
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// -- VIEWPORT & SCISSOR --
		//Create a viewport info struct
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//Create a scissor info struct
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;

		// -- DYNAMIC STATES --
		//Dynamic states to enable
		/*std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	//Dynamic viewport : Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	//Dynamic scissor : Can rsize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//Dynamic state creation info
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();*/

		// -- RASTERIZER --
		VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
		rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerCreateInfo.depthClampEnable = VK_FALSE;		//Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;//Whether to discard data skip rasterizer, only suitable for pipeline without framebuffer output
		rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerCreateInfo.lineWidth = 1.0f;
		rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

		// -- MULTISAMPLING --
		VkPipelineMultisampleStateCreateInfo multisamplineCreateInfo = {};
		multisamplineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplineCreateInfo.sampleShadingEnable = VK_FALSE;
		multisamplineCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// -- BLENDING --

		//Blend Attachment State (how blending is handled)
		VkPipelineColorBlendAttachmentState colourState = {};
		colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	//Colours to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourState.blendEnable = VK_TRUE;			//Enable blending

		colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourState.colorBlendOp = VK_BLEND_OP_ADD;	

		//Summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//			  (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)
		
		colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourState.alphaBlendOp = VK_BLEND_OP_ADD;
		//Summarized: (1 * new alpha) + (0 * old colour) = new alpha

		VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
		colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlendingCreateInfo.logicOpEnable = VK_FALSE;	//Alternative to calculations is to use logical operations
		colourBlendingCreateInfo.attachmentCount = 1;
		colourBlendingCreateInfo.pAttachments = &colourState;

		// -- PIPELINE LAYOUT --

		std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		// -- DEPTH STENCIL TESTING --
		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;	//Does depth value exists between bounds
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.front = {};
		depthStencilCreateInfo.back = {};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;
		pipelineCreateInfo.subpass = 0;
		//Pipeline derivatives: Can create multiple pi;elines that derive from one another for optimisation
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	//Existing pipeline to derive from...
		pipelineCreateInfo.basePipelineIndex = -1;	//or index of pipeline being created to derive from (in case creating multiple at once)

		//Create graphics pipeline
		result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a graphics pipeline!");
		}

		//Destroy shader modules, no longer neede after Pipeline created
		vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);

		//CREATE SECOND PASS PIPELINE
		//Second pass shaders
		auto secondVertexShaderCode = readFile("Shaders/second_vert.spv");
		auto secondFragmentShaderCode = readFile("Shaders/second_frag.spv");

		//Build shaders
		VkShaderModule secondVertexShaderModule = createShaderModule(secondVertexShaderCode);
		VkShaderModule secondFragmentShaderModule = createShaderModule(secondFragmentShaderCode);

		//Set new shaders
		vertexShaderCreateInfo.module = secondVertexShaderModule;
		fragmentShaderCreateInfo.module = secondFragmentShaderModule;

		VkPipelineShaderStageCreateInfo secondShaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };
		
		//No vertex data for second pass
		vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

		//Don't want to write to depth buffer
		depthStencilCreateInfo.depthWriteEnable = VK_FALSE;

		//New pipeline layout
		VkPipelineLayoutCreateInfo secondPipelineLayoutCreateInfo = {};
		secondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		secondPipelineLayoutCreateInfo.setLayoutCount = 1;
		secondPipelineLayoutCreateInfo.pSetLayouts = &inputSetLayout;
		secondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		secondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		result = vkCreatePipelineLayout(mainDevice.logicalDevice, &secondPipelineLayoutCreateInfo, nullptr, &secondPipelineLayout);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a second pipeline layout!");
		}

		pipelineCreateInfo.pStages = secondShaderStages;	//Update second shader stage list
		pipelineCreateInfo.layout = secondPipelineLayout;	//Chang pipeline layout for input attachment descriptor sets
		pipelineCreateInfo.subpass = 1;	//Second pass

		result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &secondPipeline);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a second graphics pipeline!");
		}

		vkDestroyShaderModule(mainDevice.logicalDevice, secondFragmentShaderModule, nullptr);
		vkDestroyShaderModule(mainDevice.logicalDevice, secondVertexShaderModule, nullptr);
	}

	void VulkanRenderer::createColourBufferImage()
	{
		colourBufferImage.resize(swapChainImages.size());
		colourBufferImageMemory.resize(swapChainImages.size());
		colourBufferImageView.resize(swapChainImages.size());

		//Get supported format for colour attachment
		VkFormat colourFormat = chooseSupportedFormat(
			{ VK_FORMAT_R8G8B8A8_UNORM },
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			//Create colour buffer image
			colourBufferImage[i] = createImage(swapChainExtent.width, swapChainExtent.height, colourFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &colourBufferImageMemory[i]);

			//Create Colour Image View
			colourBufferImageView[i] = createImageView(colourBufferImage[i], colourFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void VulkanRenderer::createDepthBufferImage()
	{
		depthBufferImage.resize(swapChainImages.size());
		depthBufferImageMemory.resize(swapChainImages.size());
		depthBufferImageView.resize(swapChainImages.size());

		//Get supported format for depth buffer
		VkFormat depthFormat = chooseSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			//Create depth buffer image
			depthBufferImage[i] = createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory[i]);

			//Create Depth Image View
			depthBufferImageView[i] = createImageView(depthBufferImage[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		}
	}

	void VulkanRenderer::createFrameBuffers()
	{
		//Resize framebuffer count to equal swap chain image count
		swapChainFrameBuffers.resize(swapChainImages.size());

		//Create a framebuffer for each swap chain image
		for (size_t i = 0; i < swapChainFrameBuffers.size(); i++)
		{
			std::array<VkImageView, 3> attachments =
			{
				swapChainImages[i].imageView,
				colourBufferImageView[i],
				depthBufferImageView[i]
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = renderPass;	//Render pass layout the Framebuffer will be used with
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferCreateInfo.pAttachments = attachments.data();	//List of attachments (1:1 with Render Pass)
			framebufferCreateInfo.width = swapChainExtent.width;
			framebufferCreateInfo.height = swapChainExtent.height;
			framebufferCreateInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapChainFrameBuffers[i]);
			if (result != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create a Frame Buffer!");
			}
		}
	}

	void VulkanRenderer::createCommandPool()
	{
		//Get indices of queue families from device;
		QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

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
		commandBuffers.resize(swapChainFrameBuffers.size());

		VkCommandBufferAllocateInfo cbAllocInfo = {};
		cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbAllocInfo.commandPool = graphicsCommandPool;
		cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	//VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer you submit directly to queue. Can't be called by other buffers.
																//VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Command Buffers!");
		}
	}

	void VulkanRenderer::createUniformBuffers()
	{
		//ViewProjection buffer size will be size of all three variables (will offset to access)
		VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

		//Model buffer size
		//VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

		//One uniform buffer for each image (and by extension, command buffer)
		vpUniformBuffer.resize(swapChainImages.size());
		vpUniformBufferMemory.resize(swapChainImages.size());

		//modelDUniformBuffer.resize(swapChainImages.size());
		//modelDUniformBufferMemory.resize(swapChainImages.size());

		//Create uniform buffers
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);
			/*createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);*/
		}
	}

	void VulkanRenderer::createDescriptorPool()
	{
		//CREATE UNIFORM DESCRIPTOR POOL
		
		//Type of descriptors + how many descriptors, not descriptor sets (combined makes the pool size)
		//ViewProjection Pool
		VkDescriptorPoolSize vpPoolSize = {};
		vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

		//Model Pool (dynamic)
		/*VkDescriptorPoolSize modelPoolSize = {};
		modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());*/

		//List of pool sizes
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

		//Data to create descriptor pool
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());	//Maximum number of descriptor sets that can be created from pool
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());	//Amount of pool sizes being passed
		poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();	//Pool sizes to create pool with

		//Create descriptor pool
		VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}

		//CREATE SAMPLER DESCRIPTOR POOL
		//Texture sampler pool
		VkDescriptorPoolSize samplerPoolSize = {};
		samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerPoolSize.descriptorCount = MAX_OBJECTS;

		VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
		samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		samplerPoolCreateInfo.maxSets = MAX_OBJECTS;	//Maximum number of descriptor sets that can be created from pool
		samplerPoolCreateInfo.poolSizeCount = 1;	//Amount of pool sizes being passed
		samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;	//Pool sizes to create pool with

		result = vkCreateDescriptorPool(mainDevice.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Sampler Descriptor Pool!");
		}

		//CREATE INPUT ATTACHMENT DESCRIPTOR POOL
		//Colour Attachment Pool Size
		VkDescriptorPoolSize colourInputPoolSize = {};
		colourInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		colourInputPoolSize.descriptorCount = static_cast<uint32_t>(colourBufferImageView.size());

		//Depth Attachment Pool Size
		VkDescriptorPoolSize depthInputPoolSize = {};
		depthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		depthInputPoolSize.descriptorCount = static_cast<uint32_t>(depthBufferImageView.size());

		std::vector<VkDescriptorPoolSize> inputPoolSizes = { colourInputPoolSize, depthInputPoolSize };

		//Create input attachment pool
		VkDescriptorPoolCreateInfo inputPoolCreateInfo = {};
		inputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		inputPoolCreateInfo.maxSets = swapChainImages.size();
		inputPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(inputPoolSizes.size());
		inputPoolCreateInfo.pPoolSizes = inputPoolSizes.data();

		result = vkCreateDescriptorPool(mainDevice.logicalDevice, &inputPoolCreateInfo, nullptr, &inputDescriptorPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Input Descriptor Pool!");
		}
	}

	void VulkanRenderer::createDescriptorSets()
	{
		//Resize descriptor set list so one for every buffer
		descriptorSets.resize(swapChainImages.size());

		std::vector<VkDescriptorSetLayout> setLayouts = { swapChainImages.size(), descriptorSetLayout };

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = descriptorPool;	//Pool to allocate descriptor set from
		setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());	//Number of sets to allocate
		setAllocInfo.pSetLayouts = setLayouts.data();	//Layouts to use to allocate sets (1:1 reslationship)

		//Allocate descriptor sets (multiple)
		VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, descriptorSets.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}

		//Update all of descriptor sets buffer binding
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			//VIEWPROJECTION DESCRIPTOR
			//Buffer info and data offset info
			VkDescriptorBufferInfo vpBufferInfo = {};
			vpBufferInfo.buffer = vpUniformBuffer[i];	//Buffer to get data from
			vpBufferInfo.offset = 0;
			vpBufferInfo.range = sizeof(UboViewProjection);

			VkWriteDescriptorSet vpSetWrite = {};
			vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vpSetWrite.dstSet = descriptorSets[i];		//Descriptor Set to update
			vpSetWrite.dstBinding = 0;		//Binding to update (matches with binding on layout/shader)
			vpSetWrite.dstArrayElement = 0;	//Index in array to update
			vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	//Type of descriptor
			vpSetWrite.descriptorCount = 1;	//Amount to update
			vpSetWrite.pBufferInfo = &vpBufferInfo;	//Information about buffer data to bind

			//MODEL DESCRIPTOR
			//Model Buffer info and data offset info
			/*VkDescriptorBufferInfo modelBufferInfo = {};
			modelBufferInfo.buffer = modelDUniformBuffer[i];	//Buffer to get data from
			modelBufferInfo.offset = 0;
			modelBufferInfo.range = modelUniformAlignment;

			VkWriteDescriptorSet modelSetWrite = {};
			modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			modelSetWrite.dstSet = descriptorSets[i];		//Descriptor Set to update
			modelSetWrite.dstBinding = 1;		//Binding to update (matches with binding on layout/shader)
			modelSetWrite.dstArrayElement = 0;	//Index in array to update
			modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;	//Type of descriptor
			modelSetWrite.descriptorCount = 1;	//Amount to update
			modelSetWrite.pBufferInfo = &modelBufferInfo;	//Information about buffer data to bind*/

			std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

			//Update descriptor sets with new buffer/binding info
			vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()),
				setWrites.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::createInputDescriptorSets()
	{
		//Resize descriptor set array for each swap chain image
		inputDescriptorSets.resize(swapChainImages.size());

		//Fill array of layouts ready for set creation
		std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), inputSetLayout);

		//Input Attachment Descriptor Set Allocation Info
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = inputDescriptorPool;
		setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
		setAllocInfo.pSetLayouts = setLayouts.data();

		//Allocate Descriptor Sets
		VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, inputDescriptorSets.data());
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Infput Attachment Descriptor Sets!");
		}

		//Update each descriptor set with input attachment
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			//Colour Attachment Descriptor
			VkDescriptorImageInfo colourAttachmentDescriptor = {};
			colourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Image layout when in use
			colourAttachmentDescriptor.imageView = colourBufferImageView[i];		//Image to bind to set
			colourAttachmentDescriptor.sampler = VK_NULL_HANDLE;		//We don't need a sampler, because we will read it in other way in shader

			//Colour Attachment Descriptor Write
			VkWriteDescriptorSet colourWrite = {};
			colourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			colourWrite.dstSet = inputDescriptorSets[i];		//Descriptor Set to update
			colourWrite.dstBinding = 0;		//Binding to update (matches with binding on layout/shader)
			colourWrite.dstArrayElement = 0;	//Index in array to update
			colourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;	//Type of descriptor
			colourWrite.descriptorCount = 1;	//Amount to update
			colourWrite.pImageInfo = &colourAttachmentDescriptor;	//Information about image data to bind
			
			//Depth Attachment Descriptor
			VkDescriptorImageInfo depthAttachmentDescriptor = {};
			depthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Image layout when in use
			depthAttachmentDescriptor.imageView = depthBufferImageView[i];		//Image to bind to set
			depthAttachmentDescriptor.sampler = VK_NULL_HANDLE;		//We don't need a sampler, because we will read it in other way in shader

			//Depth Attachment Descriptor Write
			VkWriteDescriptorSet depthWrite = {};
			depthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			depthWrite.dstSet = inputDescriptorSets[i];		//Descriptor Set to update
			depthWrite.dstBinding = 1;		//Binding to update (matches with binding on layout/shader)
			depthWrite.dstArrayElement = 0;	//Index in array to update
			depthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;	//Type of descriptor
			depthWrite.descriptorCount = 1;	//Amount to update
			depthWrite.pImageInfo = &depthAttachmentDescriptor;	//Information about image data to bind

			//List of input descriptor writes
			std::vector<VkWriteDescriptorSet> setWrites = { colourWrite, depthWrite };

			//Update descriptor sets with new buffer/binding info
			vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
	{
		//Copy VP data
		void* data;
		vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
		memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
		vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);

		//Copy Model data
		/*for (size_t i = 0; i < meshList.size(); i++)
		{
			Model* thisModel = (Model*)((uint64_t)modetTransferSpace + i * modelUniformAlignment);
			*thisModel = meshList[i].getModel();
		}
		//Map the list of model data
		vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
		memcpy(data, modetTransferSpace, modelUniformAlignment * meshList.size());
		vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);*/
	}

	void VulkanRenderer::recordCommands(uint32_t currentImage)
	{
		//Information about how to begin each command buffer
		VkCommandBufferBeginInfo bufferBeginInfo = {};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//Information about how to begin render pass (only need for graphical applications)
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };	//Start point in pixels
		renderPassBeginInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 3> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].color = { 0.6f, 0.65f, 0.4f, 1.0f };
		clearValues[2].depthStencil.depth = 1.0f;

		renderPassBeginInfo.pClearValues = clearValues.data();
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

		renderPassBeginInfo.framebuffer = swapChainFrameBuffers[currentImage];
		//Start recording commands to command buffer
		VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to start recording a Command Buffer!");
		}

			vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

				for (size_t j = 0; j < modelList.size(); j++)
				{
					MeshModel thisModel = modelList[j];
					const auto& modelMatrix = thisModel.getModel();
					//"Push" constants to given hader stage directly (no buffer)
					vkCmdPushConstants(
						commandBuffers[currentImage],
						pipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT,
						0,
						sizeof(Model),	//Size of data being pushed
						&modelMatrix);	//Actual data being pushed (can be array)

					for (size_t k = 0; k < thisModel.getMeshCount(); k++)
					{
						VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer()};	//Buffers to bind
						VkDeviceSize offsets[] = { 0 };		//Offsets into buffers being bound
						vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers, offsets);	//Command to bind vertex buffer before drawing with them

						vkCmdBindIndexBuffer(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);	//Command to bind index buffer before drawing with them

						//Dynamic offset amount
						//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;


						std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage], samplerDescriptorSets[thisModel.getMesh(k)->getTexId()] };

						vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineLayout, 0, static_cast<uint32_t>(descriptorSetGroup.size()),
							descriptorSetGroup.data(), 0, nullptr);

						vkCmdDrawIndexed(commandBuffers[currentImage], static_cast<uint32_t>(thisModel.getMesh(k)->getIndexCount()), 1, 0, 0, 0);
					}
				}

				//Start second subpass
				vkCmdNextSubpass(commandBuffers[currentImage], VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipeline);
				vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, secondPipelineLayout,
					0, 1, &inputDescriptorSets[currentImage], 0, nullptr);
				vkCmdDraw(commandBuffers[currentImage], 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[currentImage]);

		//Stop recording
		result = vkEndCommandBuffer(commandBuffers[currentImage]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to stop recording a Command Buffer!");
		}
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
		/*modelUniformAlignment = (sizeof(Model) + minUniformBufferOffset - 1)
			& ~(minUniformBufferOffset - 1);

		//Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
		modetTransferSpace = (Model*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);*/
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
		

		QueueFamilyIndices indices = getQueueFamilies(device);

		bool extensionSupported = checkDeviceExtentionSupport(device);

		bool swapChainValid = false;
		if (extensionSupported)
		{
			SwapChainDetails swapChainDetails = getSwapChainDetails(device);
			swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
		}

		return indices.isValid() && extensionSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		//Get all Queue Family properties info for the given device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

		//Go through each queue family and check if it has at least 1 of the required types of queue
		int i = 0;
		for (const auto& queueFamily : queueFamilyList)
		{
			//First check if queue family has at least 1 queue in that family (could have no queues)
			//Queue can be of multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if it has required type
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i; //If queue family invalid get index
			}

			//Check if queue family supports presentation
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
			//Check if queue is presentation type (can be both graphics and presentation)
			if (queueFamily.queueCount > 0 && presentationSupport)
			{
				indices.presentationFamily = i;
			}

			//Check if queue family indices are in a valid state, stop searching if so
			if (indices.isValid())
			{
				break;
			}
			i++;
		}

		return indices;
	}

	SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
	{
		SwapChainDetails swapChainDetails;

		//-- CAPABILITIES --
		//Get the surface capabilities for the given surface on the given physical device
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

		//-- FORMATS --
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			swapChainDetails.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
		}

		//-- PRESENTATION MODES --
		uint32_t presentationCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

		if (presentationCount != 0)
		{
			swapChainDetails.presentationModes.resize(presentationCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
		}

		return swapChainDetails;
	}

	//Best format is subjective, but ours will be:
	//format : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
	//colorSpace : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		//If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		//If restricted, search for optimal format
		for (const auto& format : formats)
		{
			if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
				&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		//If can't find optimal format, then just return first format
		return formats[0];
	}

	VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
	{
		//Look for Mailbox presentation mode
		for (const auto& presentationMode : presentationModes)
		{
			if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentationMode;
			}
		}

		//Default mode that is always available in Vulkan
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surfaceCapabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D newExtent = {};
			newExtent.width = static_cast<uint32_t>(width);
			newExtent.height = static_cast<uint32_t>(height);

			//Surface also defines max and min, so make sure within boundaries be clampeing value
			newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
			newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

			return newExtent;
		}
	}

	VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		//Loop through options an find compatible one
		for (VkFormat format : formats)
		{
			//Get properties for given format on this device
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(mainDevice.physicalDevice, format, &properties);

			//Depending on tiling choise need to check for different bit flag
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
		}
		
		throw std::runtime_error("Failed to find a matching format!");
	}

	VkImage VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
	{
		//Create Image
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	//Layout of image on creation
		imageCreateInfo.usage = useFlags;	//What image will be used for
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	//Can't be shared between queues

		VkImage image;
		VkResult result = vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create an Image!");
		}

		//Create memory for image

		//Here we ask for memory requirements for this image
		VkMemoryRequirements memoryRequirement;
		vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirement);

		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirement.size;
		memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memoryRequirement.memoryTypeBits, propFlags);

		result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate memory for Image!");
		}

		//Connect memory to image
		vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0);

		return image;
	}

	VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image;		//Image to create view for
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	//Allows remapping of RGBA components to other components
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//Subresource allow the view to view only a part of an image
		viewCreateInfo.subresourceRange.aspectMask = aspectFlags;		//Which aspect of image to view (COLOR_BIT, etc.)
		viewCreateInfo.subresourceRange.baseMipLevel = 0;				//Starting mip-level to view image from
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;				//Starting array layer to view from
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create an Image view!");
		}

		return imageView;
	}

	VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	void VulkanRenderer::createSynchronisation()
	{
		imageAvailable.resize(MAX_FRAME_DRAWS);
		renderFinished.resize(MAX_FRAME_DRAWS);
		drawFences.resize(MAX_FRAME_DRAWS);

		//Semaphore creation information
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		//Fence creation information
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
		{
			if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
				vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS ||
				vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
			}
		}
	}

	void VulkanRenderer::createTextureSampler()
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; //For border clamp only
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;	//normalized: 0-1 space, unnormalized: 0-imageSize
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;	//Level of details bias for mip level
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16.0f;	//Anisotropy sample level

		VkResult result = vkCreateSampler(mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Texture Sampler!");
		}
	}

	int VulkanRenderer::createTextureImage(std::string fileName)
	{
		//Load image file
		int width, height;
		VkDeviceSize imageSize;
		stbi_uc* imageData = loatTextureFile(fileName, &width, &height, &imageSize);

		//Create staging buffer to hold loaded data, ready to copy to device
		VkBuffer imageStagingBuffer;
		VkDeviceMemory imageStagingBufferMemory;
		createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&imageStagingBuffer, &imageStagingBufferMemory);

		//Copy image data to staging buffer
		void* data;
		vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, imageData, static_cast<size_t>(imageSize));
		vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);
		stbi_image_free(imageData);

		//Create image to hold final texture
		VkImage texImage;
		VkDeviceMemory texImageMemory;
		texImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

		//Copy data to image
		//Transition image to be DST for copy operation
		transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyImageBuffer(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width, height);

		//Transition image to be shader readable for shader usage
		transitionImageLayout(mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		textureImages.push_back(texImage);
		textureImageMemory.push_back(texImageMemory);

		//Destroy staging buffers
		vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

		//Return image index
		return textureImages.size() - 1;
	}

	int VulkanRenderer::createTexture(std::string fileName)
	{
		int textureImageLoc = createTextureImage(fileName);

		VkImageView imageView = createImageView(textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
		textureImageViews.push_back(imageView);

		//Create Texture Descriptor
		int descriptorLoc = createTextureDescriptor(imageView);

		return descriptorLoc;
	}

	int VulkanRenderer::createTextureDescriptor(VkImageView textureImage)
	{
		VkDescriptorSet descriptorSet;

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorPool = samplerDescriptorPool;	//Pool to allocate descriptor set from
		setAllocInfo.descriptorSetCount = 1;	//Number of sets to allocate
		setAllocInfo.pSetLayouts = &samplerSetLayout;	//Layouts to use to allocate sets (1:1 reslationship)

		VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, &descriptorSet);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Sampler Descriptor Sets!");
		}

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Image layout when in use
		imageInfo.imageView = textureImage;		//Image to bind to set
		imageInfo.sampler = textureSampler;		//Sampler to use for set

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;		//Descriptor Set to update
		descriptorWrite.dstBinding = 0;		//Binding to update (matches with binding on layout/shader)
		descriptorWrite.dstArrayElement = 0;	//Index in array to update
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;	//Type of descriptor
		descriptorWrite.descriptorCount = 1;	//Amount to update
		descriptorWrite.pImageInfo = &imageInfo;	//Information about image data to bind

		//Update descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

		samplerDescriptorSets.push_back(descriptorSet);

		return samplerDescriptorSets.size() - 1;
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

		//Get vector of all materials with 1:1 ID placement
		std::vector<std::string> textureNames = MeshModel::loadMaterials(scene);

		//Convertion from material IDs to our Descriptor Array IDs
		std::vector<int> matToTex(textureNames.size());

		//Loop over textureNames and create textures for them
		for (size_t i = 0; i < textureNames.size(); i++)
		{
			if (textureNames[i].empty())
			{
				//Map to default texture
				matToTex[i] = 0;
			}
			else
			{
				matToTex[i] = createTexture(textureNames[i]);
			}
		}

		//Load all meshes
		std::vector<Mesh> modelMeshes = MeshModel::loadNode(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool,
			scene->mRootNode, scene, matToTex);

		MeshModel meshModel = MeshModel(modelMeshes);
		modelList.push_back(meshModel);

		return modelList.size() - 1;
	}

	stbi_uc* VulkanRenderer::loatTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
	{
		//Number of channels image uses
		int channels;

		//Load pixel data of image
		std::string fileLoc = "Textures/" + fileName;
		stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

		if (!image)
		{
			throw std::runtime_error("Failed to load a texture file! (" + fileName + ")");
		}

		*imageSize = *width * *height * 4;

		return image;
	}
}