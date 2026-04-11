#pragma once

#include "fre/renderer/backend/vulkan/VulkanCore.hpp"

namespace fre
{
    class Fence
    {
    public:
        Fence() = default;

        Fence(vk::Device device, bool signaled = true)
            : mDevice(device)
        {
            vk::FenceCreateInfo info{};
            if(signaled)
                info.flags = vk::FenceCreateFlagBits::eSignaled;

            mFence = vkCheck(mDevice.createFence(info));
        }

        ~Fence()
        {
            if(mFence)
                mDevice.destroyFence(mFence);
        }

        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;

        Fence(Fence&& other) noexcept
        {
            *this = std::move(other);
        }

        Fence& operator=(Fence&& other) noexcept
        {
            if(this != &other)
            {
                cleanup();

                mDevice = other.mDevice;
                mFence = other.mFence;

                other.mFence = nullptr;
            }
            return *this;
        }

        void wait(uint64_t timeout = UINT64_MAX)
        {
            vkCheck(mDevice.waitForFences(1, &mFence, VK_TRUE, timeout));
        }

        void reset()
        {
            vkCheck(mDevice.resetFences(1, &mFence));
        }

        vk::Fence get() const { return mFence; }

    private:
        void cleanup()
        {
            if(mFence)
                mDevice.destroyFence(mFence);
        }

    private:
        vk::Device mDevice{};
        vk::Fence mFence{};
    };
}