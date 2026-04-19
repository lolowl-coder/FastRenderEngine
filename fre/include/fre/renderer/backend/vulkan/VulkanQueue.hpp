#pragma once

namespace fre
{
    struct QueueSelection
    {
        uint32_t familyIndex = UINT32_MAX;
        uint32_t queueIndex = UINT32_MAX; // usually 0 for now
    };

    class VulkanQueue
    {
    public:
        VulkanQueue(
            vk::Device device,
            uint32_t familyIndex,
            uint32_t queueIndex = 0)
            : mFamilyIndex(familyIndex)
            , mQueueIndex(queueIndex)
        {
            mQueue = device.getQueue(familyIndex, queueIndex);
        }

        vk::Queue get() const { return mQueue; }

        uint32_t family() const { return mFamilyIndex; }
        uint32_t index() const { return mQueueIndex; }

        void submit(
            const vk::SubmitInfo& submitInfo,
            vk::Fence fence = {})
        {
            mQueue.submit(submitInfo, fence);
        }

        void present(const vk::PresentInfoKHR& presentInfo)
        {
            mQueue.presentKHR(presentInfo);
        }

        void waitIdle()
        {
            mQueue.waitIdle();
        }

    private:
        vk::Queue mQueue;
        uint32_t mFamilyIndex;
        uint32_t mQueueIndex;
    };
}