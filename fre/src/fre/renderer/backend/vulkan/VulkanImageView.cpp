#include "fre/renderer/backend/vulkan/VulkanCore.hpp"
#include "fre/renderer/backend/vulkan/VulkanImage.hpp"
#include "fre/renderer/backend/vulkan/VulkanImageView.hpp"

namespace fre
{
    VulkanImageView::VulkanImageView(vk::Device& device, vk::Image imageHandle, IGpuImageView::Desc desc)
        : mDevice(device)
        , mImageHandle(imageHandle)
        , mDesc(desc)
    {
        vk::ImageViewCreateInfo viewInfo(
            {},
            mImageHandle,
            vk::ImageViewType::e2D,
            toVk(mDesc.mFormat),
            {},
            { toVk(mDesc.mAspect), mDesc.mBaseMip, mDesc.mMipCount, mDesc.mBaseLayer, mDesc.mLayerCount }
        );
        mView = vkCheck(mDevice.createImageView(viewInfo));
    }

    void VulkanImageView::cleanup()
    {
        if(mView)
        {
            mDevice.destroyImageView(mView);
            mView = VK_NULL_HANDLE;
        }
    }

    VulkanImageView::~VulkanImageView()
    {
        cleanup();
    }
}