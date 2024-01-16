#include "Renderer/VulkanDescriptorPool.hpp"

#include <stdexcept>

namespace fre
{
    void VulkanDescriptorPool::create(
		VkDevice logicalDevice,
		uint32_t size,
		std::vector<VkDescriptorType> descriptorTypes)
    {
        //CREATE UNIFORM DESCRIPTOR POOL
		
		//Type of descriptors + how many descriptors, not descriptor sets (combined makes the pool size)
		//ViewProjection Pool
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.resize(descriptorTypes.size());
		for(uint32_t i = 0; i < poolSizes.size(); i++)
		{
			poolSizes[i].type = descriptorTypes[i];
			poolSizes[i].descriptorCount = size;
		}

		//ModelMatrix Pool (dynamic)
		/*VkDescriptorPoolSize modelPoolSize = {};
		modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());*/

		//Data to create descriptor pool
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        //Maximum number of descriptor sets that can be created from pool
		poolCreateInfo.maxSets = size;
        //Amount of pool sizes being passed
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        //Pool sizes to create pool with
		poolCreateInfo.pPoolSizes = poolSizes.data();

		//Create descriptor pool
		VkResult result = vkCreateDescriptorPool(logicalDevice, &poolCreateInfo, nullptr, &mDescriptorPool);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}
    }

    void VulkanDescriptorPool::destroy(VkDevice logicalDevice)
    {
        vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
    }
}