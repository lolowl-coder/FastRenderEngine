#include "Renderer/VulkanDescriptorSet.hpp"

#include <stdexcept>

namespace fre
{
    void VulkanDescriptorSet::allocate(
        VkDevice logicalDevice,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout)
    {
		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        //Pool to allocate descriptor set from
		setAllocInfo.descriptorPool = descriptorPool;
        //Number of sets to allocate
		setAllocInfo.descriptorSetCount = 1;
        //Layouts to use to allocate sets (1:1 reslationship)
		setAllocInfo.pSetLayouts = &descriptorSetLayout;

		//Allocate descriptor sets (multiple)
		VkResult result = vkAllocateDescriptorSets(
            logicalDevice,
            &setAllocInfo,
            &mDescriptorSet);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}
    }

    void VulkanDescriptorSet::update(VkDevice logicalDevice, const std::vector<VkImageView>& imageViews,
        VkDescriptorType descriptorType,
        VkSampler sampler)
    {
        //Update descriptor set's buffer binding
        //Buffer info and data offset info
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.resize(imageViews.size(), {});

        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.resize(imageViews.size(), {});
        for(uint32_t i = 0; i < imageViews.size(); i++)
        {
            //Image view to get data from
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	//Image layout when in use
			imageInfos[i].imageView = imageViews[i];		//Image to bind to set
			imageInfos[i].sampler = sampler;

            //VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            //Descriptor Set to update
            writeDescriptorSets[i].dstSet = mDescriptorSet;
            //Binding to update (matches with binding on layout/shader)
            writeDescriptorSets[i].dstBinding = i;
            //Index in array to update
            writeDescriptorSets[i].dstArrayElement = 0;
            //Type of descriptor
            writeDescriptorSets[i].descriptorType = descriptorType;
            //Amount to update
            writeDescriptorSets[i].descriptorCount = 1;
            //Information about buffer data to bind
            writeDescriptorSets[i].pImageInfo = &imageInfos[i];
		}
        
        //Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
    }

    void VulkanDescriptorSet::update(VkDevice logicalDevice, VkBuffer buffer,
        VkDeviceSize stride)
    {
        //Update descriptor set's buffer binding
        //Buffer info and data offset info
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffer;	//Buffer to get data from
        bufferInfo.offset = 0;
        bufferInfo.range = stride;

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //Descriptor Set to update
        writeDescriptorSet.dstSet = mDescriptorSet;
        //Binding to update (matches with binding on layout/shader)
        writeDescriptorSet.dstBinding = 0;
        //Index in array to update
        writeDescriptorSet.dstArrayElement = 0;
        //Type of descriptor
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        //Amount to update
        writeDescriptorSet.descriptorCount = 1;
        //Information about buffer data to bind
        writeDescriptorSet.pBufferInfo = &bufferInfo;
        
        //Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(logicalDevice, 1,
            &writeDescriptorSet, 0, nullptr);
    }
}