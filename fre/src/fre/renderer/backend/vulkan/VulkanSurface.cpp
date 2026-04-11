#include "fre/core/Log.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/backend/vulkan/VulkanSurfaceWindows.hpp"
#include "fre/platform/GLFWWindow.hpp"

#include <GLFW/glfw3.h>

#include <format>

namespace fre
{
	VulkanSurfaceWindows::VulkanSurfaceWindows(vk::Instance vulkanInstance, const IGLFWWindow* window)
		: mInstance(vulkanInstance)
		, mWindow(window)
	{
		LOG_INFO("Create window surface");

		//Create surface (creates a surface create info struct, runs the create surface function, returns result)
		VkSurfaceKHR  vkSurface = VK_NULL_HANDLE;
		VkResult result = glfwCreateWindowSurface(mInstance, mWindow->getGLFWwindow(), nullptr, &vkSurface);

		if(result != VK_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to create a surface! Error code: {}", static_cast<uint32_t>(result)));
		}

		mSurface = vk::SurfaceKHR(vkSurface);

		LOG_INFO("Window surface created, glfw window: {}", static_cast<void*>(mWindow->getGLFWwindow()));
	}

	VulkanSurfaceWindows::~VulkanSurfaceWindows()
	{
		if(mSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
			LOG_INFO("Window surface destroyed, glfw window {}", static_cast<void*>(mWindow->getGLFWwindow()));
		}
	}

	uint32_t VulkanSurfaceWindows::width() const {
		int w, h;
		glfwGetFramebufferSize(mWindow->getGLFWwindow(), &w, &h);
		return static_cast<uint32_t>(w);
	}

	uint32_t VulkanSurfaceWindows::height() const {
		int w, h;
		glfwGetFramebufferSize(mWindow->getGLFWwindow(), &w, &h);
		return static_cast<uint32_t>(h);
	}
}