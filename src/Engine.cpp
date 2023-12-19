#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine.hpp"
#include "Renderer/VulkanRenderer.hpp"

namespace
{
    GLFWwindow* window;
    VulkanRenderer vulkanRenderer;
}

namespace fre
{
    bool Engine::init(std::string wName, const int width, const int height)
    {
        //Initialize GLFW
        glfwInit();
        //Set FGLW to not work with OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);

        return vulkanRenderer.init(window) == 0;
    }

    void Engine::run()
    {
        float angle = 0.0f;
        float deltaTime = 0.0f;
        float lastTime = 0.0f;

        int helicopter = vulkanRenderer.createMeshModel("Models/Seahawk.obj");
        
        //Loop until closed
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            float now = glfwGetTime();
            deltaTime = now - lastTime;
            lastTime = now;
            angle += 10.0f * deltaTime;
            if (angle > 360.0f)
            {
                angle -= 360.0f;
            }

            glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
            //testMat = glm::rotate(testMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            testMat = glm::scale(testMat, glm::vec3(0.2f, 0.2f, 0.2f));
            vulkanRenderer.updateModel(helicopter, testMat);

            vulkanRenderer.draw();
        }
    }

    void Engine::tick()
    {
    }

    void Engine::destroy()
    {
        vulkanRenderer.cleanup();

        //Destroy window
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}