#include "Engine.hpp"
#include "Renderer/VulkanPipeline.hpp"
#include "Renderer/VulkanRenderer.hpp"
#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <filesystem>

using namespace fre;

class MyRenderer : public VulkanRenderer
{
protected:
    virtual void createGraphicsPipelines() override
    {
        VulkanRenderer::createGraphicsPipelines();

        VulkanShader texturedVertexShader;
        texturedVertexShader.create(mainDevice.logicalDevice, "Shaders/textured.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        VulkanShader texturedFragShader;
        texturedFragShader.create(mainDevice.logicalDevice, "Shaders/textured.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        mTexturedPipeline.create(
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
                mUniformDescriptorSetLayout.mDescriptorSetLayout,
                mTextureManager.mSamplerDescriptorSetLayout.mDescriptorSetLayout
            },
            {pushConstantRange});

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
            {pushConstantRangeNearFar});

        //2 sub passes each using 1 pipeline
        mSubPassesCount = 2;
    }

    virtual void cleanupGraphicsPipelines(VkDevice logicalDevice) override
    {
        mTexturedPipeline.destroy(logicalDevice);
        mFogPipeline.destroy(logicalDevice);
    }

    virtual void onRenderModel(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
			const MeshModel& meshModel) override
    {
        const glm::mat4& modelMatrix = meshModel.getModelMatrix();
		//"Push" constants to given hader stage directly (no buffer)
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),	//Size of data being pushed
			&modelMatrix);	//Actual data being pushed (can be array)
    }

    virtual void renderSubPass(uint32_t imageIndex, uint32_t subPassIndex) override
    {
        setViewport(imageIndex);
        setScissor(imageIndex);

        switch(subPassIndex)
        {
        case 0:
            {
                bindPipeline(imageIndex, mTexturedPipeline.mPipeline);
                renderScene(imageIndex, mTexturedPipeline.mPipelineLayout);
            }
            break;
        case 1:
            {
                bindPipeline(imageIndex, mFogPipeline.mPipeline);
                glm::vec2 nearFar(mNear, mFar);
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
    VulkanPipeline mTexturedPipeline;
    VulkanPipeline mFogPipeline;
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

        mSceneId = mRenderer->createMeshModel("Models/sea.obj");

        return result && mSceneId != -1;
    }
    
    virtual void tick()
    {
        float angle = 0.0f;
        float deltaTime = 0.0f;
        double lastTime = 0.0f;

        double now = glfwGetTime();
        deltaTime = static_cast<float>(now - lastTime);
        lastTime = now;
        angle += 10.0f * deltaTime;
        if (angle > 360.0f)
        {
            angle -= 360.0f;
        }

        glm::mat4 testMat = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        //testMat = glm::rotate(testMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        testMat = glm::scale(testMat, glm::vec3(0.09f, 0.1f, 0.09f));
        mRenderer->updateModel(mSceneId, testMat);
    }

private:
    int mSceneId = -1;
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