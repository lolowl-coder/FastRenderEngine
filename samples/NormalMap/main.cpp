#include "Engine.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <filesystem>

using namespace fre;

#define MODEL_SCALE 0.75f

class MyEngine : public Engine
{
public:
    MyEngine()
    {
        mRenderer.reset(new VulkanRenderer);
    }

    virtual bool create(std::string wName, const int width, const int height) override
    {
        mModelId = mRenderer->createMeshModel("Models/knightHelmPBR/scene.gltf",
            {aiTextureType_DIFFUSE, aiTextureType_NORMALS});

        bool result = mModelId != -1;
        if(result)
        {
            result = Engine::create(wName, width, height);
        
            mCamera.setEye(glm::vec3(0.0, 0.0, -60.0));

            glm::mat4 sceneLocalMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(MODEL_SCALE));
            MeshModel* meshModel = mRenderer->getMeshModel(mModelId);
            meshModel->setModelMatrix(sceneLocalMatrix);
        }

        return result;
    }

private:
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