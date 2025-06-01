#include "VulkanDescriptor.hpp"
#include "VulkanBufferManager.hpp"
#include "VulkanTexture.hpp"
#include "VulkanAccelerationStructure.hpp"

namespace fre
{
    VkWriteDescriptorSet DescriptorBuffer::getWriter(VkDescriptorSet ds, uint32_t binding) const
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = mBuffer->mBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = ds;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = mType;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        return writeDescriptorSet;
    }

    VkWriteDescriptorSet DescriptorImage::getWriter(VkDescriptorSet ds, uint32_t binding) const
    {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = mLayout;
        imageInfo.imageView = mImageView;
        imageInfo.sampler = mSampler;
        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = ds;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = mType;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pImageInfo = &imageInfo;

        return writeDescriptorSet;
    };

    VkWriteDescriptorSet DescriptorInputAttachment::getWriter(VkDescriptorSet ds, uint32_t binding) const
    {
        VkWriteDescriptorSetAccelerationStructureKHR writeExt{};
        writeExt.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        writeExt.accelerationStructureCount = 1;
        writeExt.pAccelerationStructures = &mAccelerationStructure;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = ds;
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = mType;
        write.pNext = &writeExt;

        return write;
    }

    VkWriteDescriptorSet DescriptorAccelerationStructure::getWriter(VkDescriptorSet ds, uint32_t binding) const
    {
        VkWriteDescriptorSetAccelerationStructureKHR writeExt{};
        writeExt.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        writeExt.accelerationStructureCount = 1;
        writeExt.pAccelerationStructures = &mAccelerationStructure;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = ds;
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = mType;
        write.pNext = &writeExt;

        return write;
    }
}