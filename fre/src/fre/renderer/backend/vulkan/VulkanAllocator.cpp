#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"

namespace fre
{
    VulkanAllocator::VulkanAllocator(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        vk::Device device)
    {
        VmaVulkanFunctions functions{};
        functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo createInfo{};
        createInfo.instance = instance;
        createInfo.physicalDevice = physicalDevice;
        createInfo.device = device;
        createInfo.pVulkanFunctions = &functions;
        createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        if(vmaCreateAllocator(&createInfo, &mAllocator) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create VMA allocator");
        }
    }

    VulkanAllocator::~VulkanAllocator()
    {
        if(mAllocator != VK_NULL_HANDLE)
        {
            vmaDestroyAllocator(mAllocator);
        }
    }
}