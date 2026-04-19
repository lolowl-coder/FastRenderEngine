#pragma once

#include "fre/renderer/TextureHandle.hpp"
#include "fre/renderer/backend/vulkan/VulkanFence.hpp"
#include "fre/renderer/backend/vulkan/VulkanSemaphore.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommandBuffer.hpp"

namespace fre
{
    struct Frame
    {
        VulkanCommandBuffer mCmdBuff;
        Fence mRenderFence;
        Semaphore mImageAvailable;
        Semaphore mRenderFinished;
    };

    struct FrameContext
    {
        Frame* mFrame;
        uint32_t mFrameIndex;
    };
}