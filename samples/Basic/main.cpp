#include "Engine.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <filesystem>

using namespace fre;

#define MODEL_SCALE 0.09f

class MyEngine : public Engine
{
public:
    MyEngine()
    {
        mRenderer.reset(new VulkanRenderer);
        mRenderer->setClearColor(glm::vec4(0.77f, 0.85f, 0.88f, 0.0f));
    }

    virtual bool create(std::string wName, const int width, const int height) override
    {
        mModelId = mRenderer->createMeshModel("Models/island/scene.gltf",
            {aiTextureType_DIFFUSE, aiTextureType_NORMALS});

        bool result = mModelId != -1;
        if(result)
        {
            result = Engine::create(wName, width, height);
        
            mCamera.rotateBy(glm::vec3(14.0f, -204.0f, 0.0f));
            mCamera.setEye(glm::vec3(14.0f, -14.0f, 30.0f));

            glm::mat4 sceneLocalMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(MODEL_SCALE));
            MeshModel* meshModel = mRenderer->getMeshModel(mModelId);
            meshModel->setModelMatrix(sceneLocalMatrix);
        }

        return result;
    }
    
    virtual void tick()
    {
        Engine::tick();

        double intPart;
        float fractPart = static_cast<float>(std::modf(mTime / 360.0f, &intPart));
        float angle = fractPart * 360.0f * 10.0f;

        glm::mat4 sceneLocalMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        sceneLocalMatrix = glm::rotate(sceneLocalMatrix, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        //sceneLocalMatrix = glm::rotate(sceneLocalMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        sceneLocalMatrix = glm::scale(sceneLocalMatrix, glm::vec3(0.09f));
        //sceneLocalMatrix = glm::mat4(1.0f);
        MeshModel* meshModel = mRenderer->getMeshModel(mModelId);
        if(meshModel != nullptr)
        {
            meshModel->setModelMatrix(sceneLocalMatrix);
        }
    }

private:
    int mModelId = -1;
};

int main()
{
    //std::cout << std::filesystem::current_path() << std::endl;
    MyEngine engine;
    if(engine.create("FRE: Basic", 1700, 900))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}