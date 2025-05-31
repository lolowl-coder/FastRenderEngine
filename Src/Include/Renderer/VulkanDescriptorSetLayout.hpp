#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

namespace fre
{
    struct VulkanDescriptorSetLayoutInfo
    {
        std::vector<VkDescriptorType> mDescriptorTypes;
        std::vector<uint32_t> mBindings;
        std::vector<uint32_t> mDescriptorCount;
        std::vector<VkShaderStageFlags> mStageFlags;

        bool operator==(const VulkanDescriptorSetLayoutInfo& other) const
        {
            return 
                mDescriptorTypes == other.mDescriptorTypes &&
                mBindings == other.mBindings &&
                mDescriptorCount == other.mDescriptorCount &&
                mStageFlags == other.mStageFlags;
        }
    };

    struct VulkanDescriptorSetLayout
    {
        void create(
            VkDevice logicalDevice,
            const VulkanDescriptorSetLayoutInfo& key);
        void destroy(VkDevice logicalDevice);

        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    };
}