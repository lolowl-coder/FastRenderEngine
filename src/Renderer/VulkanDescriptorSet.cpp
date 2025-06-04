#include "Renderer/VulkanDescriptor.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanBufferManager.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream>

namespace fre
{
    void VulkanDescriptorSet::allocate(
        const VkDevice logicalDevice,
        const VkDescriptorPool descriptorPool,
        const VkDescriptorSetLayout descriptorSetLayout)
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

    void VulkanDescriptorSet::update(VkDevice logicalDevice, const std::vector<VulkanDescriptorPtr>& descriptors)
    {
        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.resize(descriptors.size(), {});
        for(int i = 0; i < descriptors.size(); i++)
        {
            const auto& descriptor = descriptors[i];
            descriptor->getWriter(mDescriptorSet, i);
        }
        
        //Update descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
    }
}

std::size_t std::hash<fre::VulkanDescriptorSetKey>::operator()(const fre::VulkanDescriptorSetKey& key) const {
    std::size_t seed = 0;
    std::hash<uint32_t> hasher;
    seed ^= hasher(key.mShaderId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hasher(key.mDPId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hasher(key.mDSLId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hasher(key.mMeshId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}