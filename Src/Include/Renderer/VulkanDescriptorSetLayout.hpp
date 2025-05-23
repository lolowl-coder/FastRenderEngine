#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

namespace fre
{
    struct VulkanDescriptorSetLayout
    {
        void create(
            VkDevice logicalDevice,
            const std::vector<VkDescriptorType>& descriptorTypes,
            const std::vector<uint32_t>& stageFlags);
        void destroy(VkDevice logicalDevice);

        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    };
}