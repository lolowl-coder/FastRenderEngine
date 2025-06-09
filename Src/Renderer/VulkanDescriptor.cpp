#include "VulkanDescriptor.hpp"
#include "VulkanBufferManager.hpp"
#include "VulkanTexture.hpp"
#include "VulkanAccelerationStructure.hpp"

namespace fre
{
    VkWriteDescriptorSet DescriptorBuffer::getWriter(VkDescriptorSet ds, uint32_t binding)
    {
        mBufferInfo.buffer = mBuffer->mBuffer;
        mBufferInfo.offset = 0;
        mBufferInfo.range = VK_WHOLE_SIZE;
        mWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mWriteDescriptorSet.dstSet = ds;
        mWriteDescriptorSet.dstBinding = binding;
        mWriteDescriptorSet.dstArrayElement = 0;
        mWriteDescriptorSet.descriptorType = mType;
        mWriteDescriptorSet.descriptorCount = 1;
        mWriteDescriptorSet.pBufferInfo = &mBufferInfo;

        return mWriteDescriptorSet;
    }

    VkWriteDescriptorSet DescriptorImage::getWriter(VkDescriptorSet ds, uint32_t binding)
    {
        mImageInfo.imageLayout = mLayout;
        mImageInfo.imageView = mImageView;
        mImageInfo.sampler = mSampler;
        mWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mWriteDescriptorSet.dstSet = ds;
        mWriteDescriptorSet.dstBinding = binding;
        mWriteDescriptorSet.dstArrayElement = 0;
        mWriteDescriptorSet.descriptorType = mType;
        mWriteDescriptorSet.descriptorCount = 1;
        mWriteDescriptorSet.pImageInfo = &mImageInfo;

        return mWriteDescriptorSet;
    };

    VkWriteDescriptorSet DescriptorAccelerationStructure::getWriter(VkDescriptorSet ds, uint32_t binding)
    {
        mWriteExt.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        mWriteExt.accelerationStructureCount = 1;
        mWriteExt.pAccelerationStructures = &mAccelerationStructure;
        mWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mWrite.dstSet = ds;
        mWrite.dstBinding = binding;
        mWrite.dstArrayElement = 0;
        mWrite.descriptorCount = 1;
        mWrite.descriptorType = mType;
        mWrite.pNext = &mWriteExt;

        return mWrite;
    }
}