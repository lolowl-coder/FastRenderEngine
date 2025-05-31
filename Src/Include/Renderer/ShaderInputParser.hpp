#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanDescriptorSetLayout.hpp"
#include "Pointers.hpp"

#include <vector>

namespace fre
{
    struct ShaderInputParser
    {
        void parseShaderInput(
            const std::vector<char>& source,
            std::vector<VulkanDescriptorSetLayoutInfo>& layouts);
    };
}