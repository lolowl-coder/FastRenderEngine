#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine.hpp"
#include "Renderer/VulkanRenderer.hpp"

namespace
{
    GLFWwindow* window;
}

namespace fre
{
    void Engine::positionWindow(const int width, const int height)
    {
        int count;
        int monitorX, monitorY;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[0]);
        glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);
        glfwSetWindowPos(window,
                 monitorX + (videoMode->width - width) / 2,
                 monitorY + (videoMode->height - height) / 2); 
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        renderer->setFramebufferResized(true);
    }

    bool Engine::create(std::string wName, const int width, const int height)
    {
        //Initialize GLFW
        glfwInit();

        //Set FGLW to not work with OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, mRenderer.get());
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        positionWindow(width, height);

        return window != nullptr && mRenderer != nullptr && mRenderer->create(window) == 0;
    }

    void Engine::run()
    {
        //Loop until closed
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            
            tick();

            if(mRenderer != nullptr)
            {
                mRenderer->draw();
            }
        }
    }

    void Engine::tick()
    {
    }

    void Engine::destroy()
    {
        if(mRenderer != nullptr)
        {
            mRenderer->destroy();
        }
        //Destroy window
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}