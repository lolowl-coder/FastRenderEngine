#pragma once

#include "Renderer/VulkanShader.hpp"

namespace fre
{
    struct Shader
    {
        VulkanShader mVertexShader;
        VulkanShader mFragmentShader;
    };
}