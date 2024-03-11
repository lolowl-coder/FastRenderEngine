#pragma once

#include "Renderer/VulkanShader.hpp"

#include <limits>

namespace fre
{
    struct Shader
    {
        uint32_t mId = std::numeric_limits<uint32_t>::max();
        
        VulkanShader mVertexShader;
        VulkanShader mFragmentShader;

        uint32_t mPipelineId = std::numeric_limits<uint32_t>::max();

        std::string mFileName;
    };
}