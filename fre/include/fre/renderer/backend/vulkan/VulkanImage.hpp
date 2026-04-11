#pragma once

#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"

namespace fre
{
    class VulkanImage : public IGpuImage
    {
    public:
        VulkanImage(const Desc& desc, VulkanAllocator* allocator);
        VulkanImage(const Desc& desc, vk::Image externalImage);

        ~VulkanImage();

        virtual Format format() const override
        {
            return static_cast<Format>(mDesc.format);
        }
        virtual glm::ivec3 dimensions() const override
        {
            return glm::ivec3(mExtent.width, mExtent.height, mExtent.depth);
        }
        virtual uint32_t mipLevels() const override { return mDesc.mipLevels; }
        virtual uint32_t layers() const override { return mDesc.layers; }
        vk::Image handle() { return mImage; }
    private:
        Desc mDesc;
        vk::Image mImage;
        vk::UniqueDeviceMemory mMemory;
        vk::Extent3D mExtent;

        VulkanAllocator* mAllocator = nullptr;
        VmaAllocation mAllocation = VK_NULL_HANDLE;
    };
}