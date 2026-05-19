#include "fre/renderer/ShaderManager.hpp"

namespace fre
{
    IShader* VulkanShaderSystem::load(const std::string& name)
    {
        auto it = mShaders.find(name);
        if(it != mShaders.end())
            return it->second.get();

        auto shader = std::make_unique<VulkanShader>();

        // load SPIR-V binaries
        auto vsCode = readFile(name + ".vert.spv");
        auto fsCode = readFile(name + ".frag.spv");

        shader->mStages.push_back(createStage(vsCode, VK_SHADER_STAGE_VERTEX_BIT));
        shader->mStages.push_back(createStage(fsCode, VK_SHADER_STAGE_FRAGMENT_BIT));

        createPipelineLayout(shader.get());

        auto ptr = shader.get();
        mShaders[name] = std::move(shader);
        return ptr;
    }
}