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

    void mouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        renderer->onTouch(static_cast<float>(xpos), static_cast<float>(ypos));
    }

    void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        renderer->onScroll(static_cast<float>(xOffset), static_cast<float>(yOffset));
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
        {
            bool movementEnabled = action == GLFW_PRESS;
            auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
            Camera& camera = renderer->getCamera();
            switch (key) {
                case GLFW_KEY_W:
                    camera.setMovement(Camera::M_FORWARD, movementEnabled);
                    break;
                case GLFW_KEY_S:
                    camera.setMovement(Camera::M_BACKWARD, movementEnabled);
                    break;
                case GLFW_KEY_A:
                    camera.setMovement(Camera::M_LEFT, movementEnabled);
                    break;
                case GLFW_KEY_D:
                    camera.setMovement(Camera::M_RIGHT, movementEnabled);
                    break;
            }
        }
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

        glfwSetCursorPos(window, width / 2, height / 2);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);

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
        static double lastTime = 0.0f;
        mTime = glfwGetTime();
        mTimeDelta = static_cast<float>(mTime - lastTime);
        lastTime = mTime;
        mRenderer->tick(mTime, mTimeDelta);
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