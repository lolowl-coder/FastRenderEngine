#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Renderer/VulkanBufferManager.hpp"

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

        void createShaderBindingTables(MainDevice& mainDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& mRayTracingPipelineProperties, VulkanBufferManager& bufferManager);

        void createRTPipeline(VkDevice logicalDevice,
            std::vector<VulkanShader*> shaders,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
            std::vector<VkPushConstantRange> pushConstantRanges);

        void destroy(VkDevice logicalDevice);

        bool isCompute() const;

        VkPipeline mPipeline = VK_NULL_HANDLE;
        VkPipelineBindPoint mBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups{};
        VulkanBuffer mRaygenShaderBindingTable;
        VulkanBuffer mMissShaderBindingTable;
        VulkanBuffer mHhitShaderBindingTable;
    };
}