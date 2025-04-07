#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

namespace fre
{
    struct MainDevice;

    struct VulkanDescriptorSet
    {
        using Ptr = std::shared_ptr<VulkanDescriptorSet>;

        void allocate(
            VkDevice logicalDevice,
            VkDescriptorPool descriptorPool,
            VkDescriptorSetLayout descriptorSetLayout);
        void update(VkDevice logicalDevice,
            const std::vector<VkImageLayout>& imageLayouts,
            const std::vector<VkImageView>& imageViews,
            const std::vector<VkDescriptorType>& descriptorTypes, const std::vector<VkSampler>& samplers);
        void update(VkDevice logicalDevice,
            const std::vector<VkBuffer>& buffers,
            const std::vector<VkDescriptorType> descriptorTypes,
            const std::vector<VkDeviceSize> sizes);

        VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
    };
}