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

    bool Engine::init(std::string wName, const int width, const int height)
    {
        renderer.reset(new VulkanRenderer());
        //Initialize GLFW
        glfwInit();

        //Set FGLW to not work with OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, renderer.get());
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        positionWindow(width, height);

        return renderer->init(window) == 0;
    }

    void Engine::run()
    {
        //Loop until closed
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            
            tick();

            renderer->draw();
        }
    }

    void Engine::tick()
    {
    }

    void Engine::destroy()
    {
        renderer->cleanup();

        //Destroy window
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}