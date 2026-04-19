#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
    class VulkanCommandPool
    {
    public:
        VulkanCommandPool(vk::Device device, uint32_t queueFamilyIndex)
            : mDevice(device)
        {
            vk::CommandPoolCreateInfo info{};
            info.queueFamilyIndex = queueFamilyIndex;
            // important: allows per-frame reset
            info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

            mPool = vkCheck(mDevice.createCommandPool(info));
        }

        ~VulkanCommandPool()
        {
            if(mPool)
                mDevice.destroyCommandPool(mPool);
        }

        vk::CommandBuffer allocatePrimary()
        {
            vk::CommandBufferAllocateInfo alloc{};
            alloc.commandPool = mPool;
            alloc.level = vk::CommandBufferLevel::ePrimary;
            alloc.commandBufferCount = 1;

            return vkCheck(mDevice.allocateCommandBuffers(alloc))[0];
        }

        void reset()
        {
            mDevice.resetCommandPool(mPool);
        }

        vk::CommandPool get() const { return mPool; }

    private:
        vk::Device mDevice;
        vk::CommandPool mPool;
    };
}