#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
    class VulkanCommandBuffer
    {
    public:
        VulkanCommandBuffer() = default;

        VulkanCommandBuffer(vk::CommandBuffer buffer)
            : mBuffer(buffer)
        {
        }

        ~VulkanCommandBuffer()
        {
        }

        VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
        VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

        VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
        {
            *this = std::move(other);
        }

        VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept
        {
            if(this != &other)
            {
                mBuffer = other.mBuffer;

                other.mBuffer = nullptr;
            }
            return *this;
        }

        void reset()
        {
            vkCheck(mBuffer.reset());
		}

        void begin(vk::CommandBufferUsageFlags flags = {})
        {
            vk::CommandBufferBeginInfo beginInfo{};
            beginInfo.flags = flags;
            vkCheck(mBuffer.begin(beginInfo));
        }

        void end()
        {
            vkCheck(mBuffer.end());
        }

        vk::CommandBuffer get() const { return mBuffer; }

        void transitionImage(
            vk::Image image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout,
            vk::AccessFlags srcAccess,
            vk::AccessFlags dstAccess,
            vk::PipelineStageFlags srcStage,
            vk::PipelineStageFlags dstStage)
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcAccessMask = srcAccess;
            barrier.dstAccessMask = dstAccess;
            barrier.image = image;

            barrier.subresourceRange = {
                vk::ImageAspectFlagBits::eColor,
                0, 1,
                0, 1
            };

            mBuffer.pipelineBarrier(
                srcStage,
                dstStage,
                {},
                nullptr,
                nullptr,
                barrier
            );
        }

        void beginRendering(
            vk::ImageView imageView,
            vk::Extent2D extent,
            vk::ClearColorValue clearColor)
        {
            vk::RenderingAttachmentInfo colorAttachment{};
            colorAttachment.imageView = imageView;
            colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttachment.clearValue = vk::ClearValue(clearColor);

            vk::RenderingInfo renderingInfo{};
            renderingInfo.renderArea = vk::Rect2D({ 0, 0 }, extent);
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;

            mBuffer.beginRendering(renderingInfo);
        }

        void endRendering()
        {
            mBuffer.endRendering();
        }

    private:
        vk::CommandBuffer mBuffer;
    };
}