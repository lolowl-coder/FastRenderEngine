#pragma once

#include "fre/renderer/RenderPassData.hpp"
#include "fre/renderer/backend/vulkan/EnumConvert.hpp"
#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"
#include "fre/renderer/backend/vulkan/VulkanImageView.hpp"

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

        void setViewport(const Viewport& viewport)
        {
            auto vkViewport = toVk(viewport);
            mBuffer.setViewport(0, vkViewport );
		}

        void setScissor(const ScissorRect& scissor)
        {
            auto vkScissor = toVk(scissor);
            mBuffer.setScissor(0, vkScissor);
		}

        void beginRendering(const RenderPassContext & ctx)
        {
            std::vector<vk::RenderingAttachmentInfo> colorAttachments;
            colorAttachments.reserve(ctx.attachments.size());

            for(size_t i = 0; i < ctx.attachments.size(); i++)
            {
                vk::RenderingAttachmentInfo attachment{};
				const auto* vkImageView = dynamic_cast<VulkanImageView*>(ctx.attachments[i]);
                attachment.imageView = vkImageView->handle();
                attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
                attachment.loadOp = vk::AttachmentLoadOp::eClear;
                attachment.storeOp = vk::AttachmentStoreOp::eStore;
                attachment.clearValue.color = { ctx.clearColor.x, ctx.clearColor.y, ctx.clearColor.z, ctx.clearColor.w };
                attachment.clearValue.depthStencil = vk::ClearDepthStencilValue(0.0f, 0.0f);

                colorAttachments.push_back(attachment);
            }

            vk::RenderingInfo renderingInfo{};
            renderingInfo.renderArea = vk::Rect2D({ 0,0 }, { ctx.extent.width, ctx.extent.height });
            renderingInfo.layerCount = 1;

            renderingInfo.colorAttachmentCount =
                static_cast<uint32_t>(colorAttachments.size());

            renderingInfo.pColorAttachments = colorAttachments.data();

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