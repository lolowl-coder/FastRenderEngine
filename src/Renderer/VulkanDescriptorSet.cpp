#include "Renderer/VulkanDescriptorSet.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream>

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
		VK_CHECK(vkAllocateDescriptorSets(
            logicalDevice,
            &setAllocInfo,
            &mDescriptorSet));

        //std::cout << "Allocate DS. Pool: " << descriptorPool << std::endl;
    }

    void VulkanDescriptorSet::update(VkDevice logicalDevice,
        const std::vector<VkImageLayout>& imageLayouts,
        const std::vector<VkImageView>& imageViews,
        const std::vector<VkDescriptorType>& descriptorTypes,
        const std::vector<VkSampler>& samplers)
    {
        //Update descriptor set's buffer binding
        //Image info and data offset info
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.resize(imageViews.size(), {});

        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.resize(imageViews.size(), {});
        for(uint32_t i = 0; i < imageViews.size(); i++)
        {
            //Image view to get data from
            imageInfos[i].imageLayout = imageLayouts[i];	//Image layout when in use
			imageInfos[i].imageView = imageViews[i];		//Image to bind to set
			imageInfos[i].sampler = samplers[i];

            //VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            //Descriptor Set to update
            writeDescriptorSets[i].dstSet = mDescriptorSet;
            //Binding to update (matches with binding on layout/shader)
            writeDescriptorSets[i].dstBinding = i;
            //Index in array to update
            writeDescriptorSets[i].dstArrayElement = 0;
            //Type of descriptor
            writeDescriptorSets[i].descriptorType = descriptorTypes[i];
            //Amount to update
            writeDescriptorSets[i].descriptorCount = 1;
            //Information about buffer data to bind
            writeDescriptorSets[i].pImageInfo = &imageInfos[i];
		}
        
        //Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
    }

    void VulkanDescriptorSet::update(VkDevice logicalDevice,
        const std::vector<VkBuffer>& buffers,
        const std::vector<VkDescriptorType> descriptorTypes,
        const std::vector<VkDeviceSize> sizes)
    {
        assert(buffers.size() == descriptorTypes.size() && buffers.size() == sizes.size());
        //Update descriptor set's buffer binding
        //Buffer info and data offset info
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.resize(buffers.size(), {});

        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.resize(buffers.size(), {});
        for(uint32_t i = 0; i < buffers.size(); i++)
        {
            //buffer to get data from
            bufferInfos[i].offset = 0;
            //Buffer to bind to set
			bufferInfos[i].buffer = buffers[i];
			bufferInfos[i].range = sizes[i];

            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            //Descriptor Set to update
            writeDescriptorSets[i].dstSet = mDescriptorSet;
            //Binding to update (matches with binding on layout/shader)
            writeDescriptorSets[i].dstBinding = i;
            //Index in array to update
            writeDescriptorSets[i].dstArrayElement = 0;
            //Type of descriptor
            writeDescriptorSets[i].descriptorType = descriptorTypes[i];
            //Amount to update
            writeDescriptorSets[i].descriptorCount = 1;
            //Information about buffer data to bind
            writeDescriptorSets[i].pBufferInfo = &bufferInfos[i];
		}
        
        //Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(logicalDevice, writeDescriptorSets.size(),
            writeDescriptorSets.data(), 0, nullptr);
    }
}