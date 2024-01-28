#include "Engine.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <filesystem>

using namespace fre;

#define MODEL_SCALE 80.0f

//shader input
struct Lighting
{
    glm::vec4 cameraEye;
    glm::vec4 lightPos;
    glm::mat4 normalMatrix;
};

class MyRenderer : public VulkanRenderer
{
protected:
    virtual void createGraphicsPipelines() override
    {
        VulkanRenderer::createGraphicsPipelines();

        mModelMatrixPCR.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//Shader stage push constant will go to
		mModelMatrixPCR.offset = 0;
		mModelMatrixPCR.size = sizeof(glm::mat4);

        mLightingPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mLightingPCR.offset = sizeof(glm::mat4);
		mLightingPCR.size = sizeof(Lighting);

        mNearFarPCR.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;	//Shader stage push constant will go to
		mNearFarPCR.offset = 0;
		mNearFarPCR.size = sizeof(glm::vec2);

        VulkanShader texturedVertexShader;
        texturedVertexShader.create(mainDevice.logicalDevice, "Shaders/normalMap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        VulkanShader texturedFragShader;
        texturedFragShader.create(mainDevice.logicalDevice, "Shaders/normalMap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        mNormalMapPipeline.create(
            mainDevice.logicalDevice,
            {texturedVertexShader, texturedFragShader},
            sizeof(Vertex),
            {
                {VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
                {VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, col)},
                {VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tex)}
            },
            VK_TRUE,
            mRenderPass.mRenderPass,
            0,
            {
                //All inputs used in render pass
                //Uniforms (model matrix) in subpass 0
                mUniformDescriptorSetLayout.mDescriptorSetLayout,
                //Color texture for subpass 0
                mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout,
                //Normals texture for subpass 0
                mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
            },
            {mModelMatrixPCR, mLightingPCR});

        VulkanShader fogVertexShader;
        fogVertexShader.create(mainDevice.logicalDevice, "Shaders/fog.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        VulkanShader fogFragShader;
        fogFragShader.create(mainDevice.logicalDevice, "Shaders/fog.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        mFogPipeline.create(
            mainDevice.logicalDevice,
            {fogVertexShader, fogFragShader},
            0,
            //no attributes for fog pass
            {},
            VK_FALSE,
            mRenderPass.mRenderPass,
            1,
            {mInputDescriptorSetLayout.mDescriptorSetLayout},
            {mNearFarPCR});

        //2 sub passes each using 1 pipeline
        mSubPassesCount = 2;
    }

    virtual void cleanupGraphicsPipelines(VkDevice logicalDevice) override
    {
        mNormalMapPipeline.destroy(logicalDevice);
        mFogPipeline.destroy(logicalDevice);
    }

    virtual void onRenderModel(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
			const MeshModel& meshModel, const Camera& camera) override
    {
        const glm::mat4& modelMatrix = meshModel.getModelMatrix();
		//"Push" constants to given shader stage directly (no buffer)
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),	//Size of data being pushed
			&modelMatrix);	//Actual data being pushed (can be array)
        float sn = std::sin(glfwGetTime()) * 0.5 + 0.5;
        float xSize = mModelMx.x - mModelMn.x;
        float x = sn * xSize + mModelMn.x;
        x = 0.0f;
        //std::cout << x << std::endl;
        glm::mat3 normalMatrix(modelMatrix);
        normalMatrix = glm::transpose(glm::inverse(normalMatrix));
        Lighting lighting;
        lighting.normalMatrix = glm::mat4(normalMatrix);
        lighting.cameraEye = glm::vec4(camera.getEye(), 0.0);
        lighting.lightPos = glm::vec4(x * MODEL_SCALE, 0.0, mModelMx.z + 20.0, 0.0);
        //std::cout << "mat3 size: " << sizeof(glm::mat3) << std::endl;
        //std::cout << "Lighting.normalMatrix: " << offsetof(Lighting, normalMatrix) << std::endl;
        //std::cout << "Lighting.cameraEye: " << offsetof(Lighting, cameraEye) << std::endl;
        //std::cout << "Lighting.lightPos: " << offsetof(Lighting, lightPos) << std::endl;
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			sizeof(glm::mat4),
			sizeof(Lighting),
			&lighting);
    }

    virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex, const Camera& camera) override
    {
        setViewport(imageIndex);
        setScissor(imageIndex);

        switch(subPassIndex)
        {
        case 0:
            {
                bindPipeline(imageIndex, mNormalMapPipeline.mPipeline);
                renderScene(imageIndex, mNormalMapPipeline.mPipelineLayout, camera);
            }
            break;
        case 1:
            {
                bindPipeline(imageIndex, mFogPipeline.mPipeline);
                glm::vec2 nearFar(camera.mNear, camera.mFar);
                vkCmdPushConstants(
                    mCommandBuffers[imageIndex].mCommandBuffer,
                    mFogPipeline.mPipelineLayout,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(glm::vec2),
                    &nearFar);
                renderTexturedRect(imageIndex, mFogPipeline.mPipelineLayout);
            }
            break;
        }
    }

private:
    VulkanPipeline mNormalMapPipeline;
    VulkanPipeline mFogPipeline;
    VkPushConstantRange mModelMatrixPCR;
    VkPushConstantRange mLightingPCR;
    VkPushConstantRange mNearFarPCR;
};

class MyEngine : public Engine
{
public:
    MyEngine()
    {
        mRenderer.reset(new MyRenderer);
    }

    virtual bool create(std::string wName, const int width, const int height) override
    {
        bool result = Engine::create(wName, width, height);
        
        mCamera.setEye(glm::vec3(0.0, 0.0, -60.0));

        if(result)
        {
            mModelId = mRenderer->createMeshModel("Models/scene.gltf");
        }

        return result && mModelId != -1;
    }
    
    virtual void tick()
    {
        Engine::tick();

        double intPart;
        float fractPart = static_cast<float>(std::modf(mTime / 360.0f, &intPart));
        float angle = fractPart * 360.0f * 10.0f;

        glm::mat4 sceneLocalMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        //sceneLocalMatrix = glm::rotate(sceneLocalMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        sceneLocalMatrix = glm::mat4(1.0);
        sceneLocalMatrix = glm::scale(sceneLocalMatrix, glm::vec3(MODEL_SCALE));
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
    if(engine.create("Test Vulkan", 1700, 900))
    {
        engine.run();
        engine.destroy();
    }
    return 0;
}