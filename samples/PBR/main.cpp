#include "Engine.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <filesystem>

using namespace fre;

class MyEngine : public Engine
{
public:
    MyEngine()
    {
        mRenderer.reset(new VulkanRenderer);
        mRenderer->setClearColor(glm::vec4(43.0f, 37.0f, 44.0f, 0.0f) / 256.0f);
    }

    virtual bool create(std::string wName, const int width, const int height) override
    {
        /*mCoordSystemModelId = mRenderer->createMeshModel("Models/coordSystem/scene.obj",
            {aiTextureType_DIFFUSE});*/
        //mModelId = mRenderer->createMeshModel("Models/earth/scene.gltf",
        mModelId = mRenderer->createMeshModel
        (
            "Models/sciFiShip/scene.gltf",
            //mModelId = mRenderer->createMeshModel("Models/coordSystem/scene.obj",
            {
                aiTextureType_BASE_COLOR,
                aiTextureType_NORMALS,
                //r - metallness, g - roughness
                aiTextureType_METALNESS
            }
        );

        bool result = mModelId != -1/* && mCoordSystemModelId != -1*/;
        if(result)
        {
            result = Engine::create(wName, width, height);
        
            mCamera.setEye(glm::vec3(0.0, 0.0, -60.0));
            mLight.mColor = glm::vec3(1.0f, 0.4f, 0.4f);

            glm::mat4 sceneLocalMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            sceneLocalMatrix = glm::scale(sceneLocalMatrix, glm::vec3(5.5));
            MeshModel* meshModel = mRenderer->getMeshModel(mModelId);
            meshModel->setModelMatrix(sceneLocalMatrix);
        }

        return result;
    }

private:
    int mCoordSystemModelId = -1;
    int mModelId = -1;
};

int main()
{
    //std::cout << std::filesystem::current_path() << std::endl;
    MyEngine engine;
    if(engine.create("Test Vulkan", 1700, 900))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}