#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;

    struct VulkanDescriptorSetLayout
    {
        void create(
            VkDevice logicalDevice,
            std::vector<VkDescriptorType> descriptorTypes,
            VkShaderStageFlagBits stageFlags);
        void destroy(VkDevice logicalDevice);

        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    };
}