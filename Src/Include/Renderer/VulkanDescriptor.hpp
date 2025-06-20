#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Pointers.hpp"

#include <vector>
#include <memory>

namespace fre
{
    struct VulkanDescriptor
    {
        VulkanDescriptor(VkDescriptorType type)
            : mType(type)
        {
        }
        VkDescriptorType mType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        virtual ~VulkanDescriptor() = default;
        virtual VkWriteDescriptorSet getWriter(VkDescriptorSet ds, uint32_t binding) = 0;
    };

    struct DescriptorBuffer : public VulkanDescriptor
    {
        DescriptorBuffer()
            : VulkanDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
        }
        VulkanBufferPtr mBuffer;
        virtual VkWriteDescriptorSet getWriter(VkDescriptorSet ds, uint32_t binding) override;
    private:
        VkDescriptorBufferInfo mBufferInfo = {};
        VkWriteDescriptorSet mWriteDescriptorSet = {};
    };

    struct DescriptorImage : public VulkanDescriptor
    {
        DescriptorImage(VkDescriptorType type, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler)
            : VulkanDescriptor(type)
            , mLayout(imageLayout)
            , mImageView(imageView)
            , mSampler(sampler)
        {
        }
        VkImageLayout mLayout = VK_IMAGE_LAYOUT_MAX_ENUM;
        VkImageView mImageView = VK_NULL_HANDLE;
        VkSampler mSampler = VK_NULL_HANDLE;
        virtual VkWriteDescriptorSet getWriter(VkDescriptorSet ds, uint32_t binding) override;

    private:
        VkDescriptorImageInfo mImageInfo = {};
        VkWriteDescriptorSet mWriteDescriptorSet = {};
    };

    struct DescriptorAccelerationStructure : public VulkanDescriptor
    {
        DescriptorAccelerationStructure(VkAccelerationStructureKHR handle)
            : VulkanDescriptor(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
            , mAccelerationStructure(handle)
        {
        }
        VkAccelerationStructureKHR mAccelerationStructure = VK_NULL_HANDLE;
        virtual VkWriteDescriptorSet getWriter(VkDescriptorSet ds, uint32_t binding) override;

    private:
        VkWriteDescriptorSetAccelerationStructureKHR mWriteExt = {};
        VkWriteDescriptorSet mWrite = {};
    };
}