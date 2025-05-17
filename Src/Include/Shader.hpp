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
            mRayGenShader.destroy(logicalDevice);
            mRayMissShader.destroy(logicalDevice);
            mRayClosestHitShader.destroy(logicalDevice);
        }

        uint32_t mId = std::numeric_limits<uint32_t>::max();
        
        //Shaders
        VulkanShader mVertexShader;
        VulkanShader mFragmentShader;
        VulkanShader mComputeShader;
        VulkanShader mRayGenShader;
        VulkanShader mRayMissShader;
        VulkanShader mRayClosestHitShader;
        //Name for debugging purposes
        std::string mName;
        //Pipeline associated with this shader
        std::vector<uint32_t> mGraphicsPipelineIds;
        std::vector<uint32_t> mComputePipelineIds;
        std::vector<uint32_t> mRTPipelineIds;
    };
}