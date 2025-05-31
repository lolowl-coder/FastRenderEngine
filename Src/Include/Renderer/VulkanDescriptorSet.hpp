#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Pointers.hpp"

#include <vector>
#include <memory>

namespace fre
{
    struct MainDevice;

    struct VulkanDescriptorSetKey
    {
        uint32_t mShaderId = std::numeric_limits<uint32_t>::max();
        uint32_t mDPId = std::numeric_limits<uint32_t>::max();
        uint32_t mDSLId = std::numeric_limits<uint32_t>::max();
        uint32_t mMeshId = std::numeric_limits<uint32_t>::max();
    };

    struct VulkanDescriptorSet
    {
        void allocate(
            const VkDevice logicalDevice,
            const VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout descriptorSetLayout);
        void update(VkDevice logicalDevice, const std::vector<VulkanDescriptorPtr>& descriptors);

        VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
    };
}