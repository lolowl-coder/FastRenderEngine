#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

namespace fre
{
    struct VulkanDescriptorPoolKey
    {
        std::vector<VkDescriptorPoolSize> mPoolSizes;

        bool operator == (const VulkanDescriptorPoolKey& other) const
        {
            return mPoolSizes == other.mPoolSizes;
        }
    };

    struct VulkanDescriptorPool
    {
        void create(
            VkDevice logicalDevice,
            uint32_t count,
            //it's possible to create pool of multiple inputs.
            //e. g. color and depth attachments in one pool.
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        void destroy(VkDevice logicalDevice);

        VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    };
}