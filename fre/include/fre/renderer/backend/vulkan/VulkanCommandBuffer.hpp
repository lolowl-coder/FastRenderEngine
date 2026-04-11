#pragma once

#include <vulkan/vulkan.hpp>

namespace fre
{
    class CommandBuffer
    {
    public:
        CommandBuffer() = default;

        CommandBuffer(vk::Device device, vk::CommandPool pool)
            : mDevice(device), mPool(pool)
        {
            vk::CommandBufferAllocateInfo allocInfo{};
            allocInfo.commandPool = mPool;
            allocInfo.level = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandBufferCount = 1;

            mCommandBuffer = mDevice.allocateCommandBuffers(allocInfo)[0];
        }

        ~CommandBuffer()
        {
            if(mCommandBuffer)
                mDevice.freeCommandBuffers(mPool, mCommandBuffer);
        }

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&& other) noexcept
        {
            *this = std::move(other);
        }

        CommandBuffer& operator=(CommandBuffer&& other) noexcept
        {
            if(this != &other)
            {
                cleanup();

                mDevice = other.mDevice;
                mPool = other.mPool;
                mCommandBuffer = other.mCommandBuffer;

                other.mCommandBuffer = nullptr;
            }
            return *this;
        }

        void begin(vk::CommandBufferUsageFlags flags = {})
        {
            vk::CommandBufferBeginInfo beginInfo{};
            beginInfo.flags = flags;
            mCommandBuffer.begin(beginInfo);
        }

        void end()
        {
            mCommandBuffer.end();
        }

        vk::CommandBuffer get() const { return mCommandBuffer; }

    private:
        void cleanup()
        {
            if(mCommandBuffer)
                mDevice.freeCommandBuffers(mPool, mCommandBuffer);
        }

    private:
        vk::Device mDevice{};
        vk::CommandPool mPool{};
        vk::CommandBuffer mCommandBuffer{};
    };
}