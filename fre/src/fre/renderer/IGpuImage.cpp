#include "fre/renderer/IGpuImage.hpp"

namespace fre
{
    vk::ImageUsageFlags toVk(ImageUsage usage)
    {
        vk::ImageUsageFlags flags{};

        if((usage & ImageUsage::TransferSrc) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransferSrc;

        if((usage & ImageUsage::TransferDst) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransferDst;

        if((usage & ImageUsage::Sampled) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eSampled;

        if((usage & ImageUsage::Storage) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eStorage;

        if((usage & ImageUsage::Color) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eColorAttachment;

        if((usage & ImageUsage::DepthStencil) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

        if((usage & ImageUsage::Input) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eInputAttachment;

        if((usage & ImageUsage::Transient) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransientAttachment;

        return flags;
    }
}