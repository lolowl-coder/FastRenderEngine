#include "Engine.hpp"
#include "Renderer/VulkanRenderer.hpp"

#include <iostream>
#include <filesystem>

class MyEngine : public fre::Engine
{
public:
    virtual bool init(std::string wName, const int width, const int height) override
    {
        bool result = Engine::init(wName, width, height);

        helicopter = renderer->createMeshModel("Models/Seahawk.obj");

        return result && helicopter != -1;
    }
    virtual void tick()
    {
        float angle = 0.0f;
        float deltaTime = 0.0f;
        float lastTime = 0.0f;

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
        renderer->updateModel(helicopter, testMat);
    }
private:
    int helicopter = -1;
};

int main()
{
    std::cout << std::filesystem::current_path() << std::endl;
    MyEngine engine;
    if(engine.init("Test Vulkan", 800, 600))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}