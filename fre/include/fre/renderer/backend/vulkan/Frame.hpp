#pragma once

#include "fre/renderer/TextureHandle.hpp"
#include "fre/renderer/backend/vulkan/VulkanFence.hpp"
#include "fre/renderer/backend/vulkan/VulkanSemaphore.hpp"

namespace fre
{
    struct Frame
    {
        //CommandBuffer mCmdBuff;
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