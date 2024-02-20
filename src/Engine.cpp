#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine.hpp"
#include "Timer.hpp"
#include "Renderer/VulkanRenderer.hpp"

#include "imgui.h"

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
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->setupCamera(width, height);
        auto& renderer = engine->getRenderer();
        renderer->setFramebufferResized(true);
    }

    static bool gMouseLeftButtonPressed = false;

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            ImGuiIO& io = ImGui::GetIO();
            gMouseLeftButtonPressed = action == GLFW_PRESS && !io.WantCaptureMouse;
            if(action == GLFW_RELEASE)
            {
                Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
                Camera& camera = engine->getCamera();
                for(uint8_t i = 0; i < Camera::EMovement::M_COUNT; i++)
                {
                    camera.setMovement(static_cast<Camera::EMovement>(i), false);
                }
            }
        }
    }

    void mouseCallback(GLFWwindow* window, double x, double y)
    {
        static float lastX = static_cast<float>(x);
        static float lastY = static_cast<float>(y);

		float deltaX = static_cast<float>(x - lastX);
        float deltaY = static_cast<float>(y - lastY);

		//std::cout << deltaX << std::endl;
        ImGuiIO& io = ImGui::GetIO();
        if(gMouseLeftButtonPressed && !io.WantCaptureMouse)
        {
            Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
            auto& camera = engine->getCamera();
            camera.rotateBy(glm::vec3(
                engine->getCameraRotationSpeed() * deltaY,
                engine->getCameraRotationSpeed() * deltaX, 0.0));
        }

		lastX = static_cast<float>(x);
        lastY = static_cast<float>(y);
    }

    void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
    {
        Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        auto& camera = engine->getCamera();
		const glm::vec3 forward = camera.getForward();
		camera.translateBy(engine->getCameraZoomSpeed() * forward * static_cast<float>(yOffset));
    }

    void setCameraMovement(Camera::EMovement movement, Camera& camera, bool movementEnabled)
    {
        if(gMouseLeftButtonPressed)
        {
            camera.setMovement(movement, movementEnabled);
        }
    }

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
        {
            bool movementEnabled = action == GLFW_PRESS;
            Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
            Camera& camera = engine->getCamera();
            switch (key) {
                case GLFW_KEY_W:
                    setCameraMovement(Camera::M_FORWARD, camera, movementEnabled);
                    break;
                case GLFW_KEY_S:
                    setCameraMovement(Camera::M_BACKWARD, camera, movementEnabled);
                    break;
                case GLFW_KEY_A:
                    setCameraMovement(Camera::M_LEFT, camera, movementEnabled);
                    break;
                case GLFW_KEY_D:
                    setCameraMovement(Camera::M_RIGHT, camera, movementEnabled);
                    break;
                case GLFW_KEY_Q:
                    setCameraMovement(Camera::M_DOWN, camera, movementEnabled);
                    break;
                case GLFW_KEY_E:
                    setCameraMovement(Camera::M_UP, camera, movementEnabled);
                    break;
                case GLFW_KEY_L:
                    Light& light = engine->getLight();
                    light.mPosition = -camera.getEye();

                    std::cout << "Light position: " << light.mPosition.x << " "
                        << light.mPosition.y << " " << light.mPosition.z << std::endl;

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
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        positionWindow(width, height);

        glfwSetCursorPos(window, width / 2, height / 2);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);

        setupCamera(width, height);

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
                mRenderer->draw(mCamera, mLight);
            }
        }
    }

    void Engine::tick()
    {
        static double lastTime = 0.0f;
        mTime = Timer::getInstance().getTime();
        mTimeDelta = static_cast<float>(mTime - lastTime);
        lastTime = mTime;
        
        mCamera.update(mTimeDelta);
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

    std::shared_ptr<VulkanRenderer>& Engine::getRenderer()
    {
        return mRenderer;
    }

    void Engine::setupCamera(int width, int height)
	{
        if(width != 0 && height != 0)
		{
            float aspectRatio = (float)width / (float)height;
		    mCamera.setPerspectiveProjection(45.0f, aspectRatio, 0.1f, 100.0f);
        }
	}

    Camera& Engine::getCamera()
    {
        return mCamera;
    }

    float Engine::getCameraRotationSpeed() const
    {
        return mCameraRotationSpeed;
    }
    float Engine::getCameraZoomSpeed() const
    {
        return mCameraZoomSpeed;
    }

    Light& Engine::getLight()
    {
        return mLight;
    }
}