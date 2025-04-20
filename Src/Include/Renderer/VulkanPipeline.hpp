#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanShader;
    struct VulkanVertexAttribute;

    struct VulkanPipeline
    {
        void createGeometryPipeline(
            VkDevice logicalDevice,
            std::vector<VulkanShader*> shaders,
            VkPrimitiveTopology topology,
            uint32_t stride,
            const std::vector<VulkanVertexAttribute>& vertexAttributes,
            VkBool32 depthWriteEnable,
            VkRenderPass renderPass,
            uint32_t subpassIndex,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
            std::vector<VkPushConstantRange> pushConstantRanges,
            uint32_t attachmentsCount,
            float lineWidth,
            VkCullModeFlags cullMode);

        void createComputePipeline(
            VkDevice logicalDevice,
            VulkanShader& shader,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		    std::vector<VkPushConstantRange> pushConstantRanges);

        void createRTPipeline

        void destroy(VkDevice logicalDevice);

        bool isCompute() const;

        VkPipeline mPipeline = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    private:
        bool mIsCompute = false;
    };
}