#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

#include <vk_mem_alloc.h>

namespace fre
{
    class VulkanAllocator
    {
    public:
        VulkanAllocator(
            vk::Instance instance,
            vk::PhysicalDevice physicalDevice,
            vk::Device device);

        ~VulkanAllocator();

        VmaAllocator get() const { return mAllocator; }

    private:
        VmaAllocator mAllocator = VK_NULL_HANDLE;
    };
}