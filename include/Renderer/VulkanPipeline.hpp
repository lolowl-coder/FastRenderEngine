#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanShader;
    struct VulkanVertexAttribute;

    struct VulkanPipeline
    {
        void create(
            VkDevice logicalDevice,
            std::vector<VulkanShader> shaders,
            uint32_t stride,
            const std::vector<VulkanVertexAttribute>& vertexAttributes,
            VkBool32 depthWriteEnable,
            VkRenderPass renderPass,
            uint32_t subpassIndex,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
            std::vector<VkPushConstantRange> pushConstantRanges);

        void destroy(VkDevice logicalDevice);

        VkPipeline mPipeline = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    private:

        std::vector<VulkanShader> mShaders; 
    };
}