#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct MainDevice;

    struct VulkanDescriptorPool
    {
        void create(
            VkDevice logicalDevice,
            uint32_t size,
            //it's possible to create pool of multiple inputs.
            //e. g. color and depth attachments in one pool.
            std::vector<VkDescriptorType> descriptorTypes);
        void destroy(VkDevice logicalDevice);

        VkDescriptorPool mDescriptorPool;
    };
}