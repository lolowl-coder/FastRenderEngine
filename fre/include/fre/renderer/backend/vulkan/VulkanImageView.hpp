#pragma once

#include "fre/core/Format.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"

namespace fre
{
    class VulkanImage;

    class VulkanImageView : public IGpuImageView
    {
    public:
        VulkanImageView(vk::Device& device, vk::Image imageHandle, Desc imageViewDesc);
        ~VulkanImageView() noexcept;

        VulkanImageView(const VulkanImageView&) = delete;
        VulkanImageView& operator=(const VulkanImageView&) = delete;

        VulkanImageView(VulkanImageView&& other) noexcept
            : mDevice(other.mDevice)
        {
            mImageHandle = other.mImageHandle;
            mView = other.mView;

            other.mDevice = VK_NULL_HANDLE;
            other.mImageHandle = VK_NULL_HANDLE;
            other.mView = VK_NULL_HANDLE;
        }
        
        VulkanImageView& operator=(VulkanImageView&& other) noexcept
        {
            if(this != &other)
            {
                cleanup();
                mDevice = other.mDevice;
                mImageHandle = other.mImageHandle;
                mView = other.mView;
                other.mDevice = VK_NULL_HANDLE;
                other.mImageHandle = VK_NULL_HANDLE;
                other.mView = VK_NULL_HANDLE;
            }
            return *this;
        }

        virtual uint32_t baseMip() const override { return mDesc.mBaseMip; }
        virtual uint32_t mipCount() const override { return mDesc.mMipCount; }

        virtual uint32_t baseLayer() const override { return mDesc.mBaseLayer; }
		virtual uint32_t layerCount() const override { return mDesc.mLayerCount; }

        virtual ComponentMapping componentMapping() const override { return mDesc.mComponentMapping; }

        vk::ImageView handle() const { return mView; }
        vk::Image image() const { return mImageHandle; }
    private:
        void cleanup();
    private:
        Desc mDesc;
		vk::Device& mDevice;
        vk::Image mImageHandle = {};
        vk::ImageView mView = {};
    };
}