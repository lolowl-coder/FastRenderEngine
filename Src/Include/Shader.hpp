#pragma once

#include "Renderer/VulkanShader.hpp"

#include <limits>

namespace fre
{
    //Vertex and fragment shader bundle
    struct Shader
    {
        void destroy(VkDevice logicalDevice)
        {
            mVertexShader.destroy(logicalDevice);
            mFragmentShader.destroy(logicalDevice);
            mComputeShader.destroy(logicalDevice);
        }

        uint32_t mId = std::numeric_limits<uint32_t>::max();
        
        //Shaders
        VulkanShader mVertexShader;
        VulkanShader mFragmentShader;
        VulkanShader mComputeShader;
        //Name for debugging purposes
        std::string mName;
        //Pipeline associated with this shader
        std::vector<uint32_t> mGraphicsPipelineIds;
        std::vector<uint32_t> mComputePipelineIds;
    };
}