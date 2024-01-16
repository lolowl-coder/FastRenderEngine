#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;

    struct VulkanDescriptorSet
    {
        void allocate(
            VkDevice logicalDevice,
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout);
        void update(VkDevice logicalDevice, const std::vector<VkImageView>& imageViews,
            VkDescriptorType descriptorType, VkSampler sampler);
        void update(VkDevice logicalDevice, VkBuffer buffer, VkDeviceSize stride);

        VkDescriptorSet mDescriptorSet;
    };
}