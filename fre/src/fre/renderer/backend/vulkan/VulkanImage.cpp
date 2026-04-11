#include "fre/renderer/backend/vulkan/VulkanImage.hpp"

namespace fre
{
	VulkanImage::VulkanImage(const Desc& desc, VulkanAllocator* allocator)
        : mDesc(desc)
        , mAllocator(allocator)
	{
        vk::ImageCreateInfo ici;
        ici.imageType = vk::ImageType::e2D;
        ici.format = toVk(mDesc.format);
        ici.extent = vk::Extent3D(mDesc.width, mDesc.height, mDesc.depth);
        ici.mipLevels = mDesc.mipLevels;
        ici.arrayLayers = mDesc.layers;
        ici.samples = vk::SampleCountFlagBits::e1;
        ici.usage = toVk(mDesc.usage);

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkImage vkImage = mImage;

        vmaCreateImage(mAllocator->get(),
            reinterpret_cast<VkImageCreateInfo*>(&ici),
            &allocInfo,
            &vkImage,
            &mAllocation,
            nullptr);
	}

    VulkanImage::VulkanImage(const Desc& desc, vk::Image externalImage)
        : mDesc(desc)
        , mImage(externalImage)
        , mAllocator(nullptr)
    {

    }

    VulkanImage::~VulkanImage()
    {
        if(mAllocation != nullptr)
        {
            vmaDestroyImage(mAllocator->get(), mImage, mAllocation);
        }
    }
}