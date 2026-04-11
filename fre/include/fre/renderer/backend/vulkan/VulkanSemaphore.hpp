#pragma once

#include "fre/renderer/backend/vulkan/VulkanCore.hpp"

namespace fre
{
    class Semaphore
    {
    public:
        Semaphore() = default;

        Semaphore(vk::Device device)
            : mDevice(device)
        {
            vk::SemaphoreCreateInfo info{};
            mSemaphore = vkCheck(mDevice.createSemaphore(info));
        }

        ~Semaphore()
        {
            if(mSemaphore)
                mDevice.destroySemaphore(mSemaphore);
        }

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&& other) noexcept
        {
            *this = std::move(other);
        }

        Semaphore& operator=(Semaphore&& other) noexcept
        {
            if(this != &other)
            {
                cleanup();

                mDevice = other.mDevice;
                mSemaphore = other.mSemaphore;

                other.mSemaphore = nullptr;
            }
            return *this;
        }

        vk::Semaphore get() const { return mSemaphore; }

    private:
        void cleanup()
        {
            if(mSemaphore)
                mDevice.destroySemaphore(mSemaphore);
        }

    private:
        vk::Device mDevice{};
        vk::Semaphore mSemaphore{};
    };
}