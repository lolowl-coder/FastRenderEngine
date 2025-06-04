#include "Renderer/VulkanDescriptorPool.hpp"
#include "Utilities.hpp"

#include <stdexcept>

namespace fre
{
    void VulkanDescriptorPool::create(
		VkDevice logicalDevice,
		uint32_t setsCount,
		const std::vector<VkDescriptorPoolSize>& poolSizes)
    {
        //CREATE UNIFORM DESCRIPTOR POOL
		
		//Type of descriptors + how many descriptors, not descriptor sets (combined makes the pool size)

		//ModelMatrix Pool (dynamic)
		/*VkDescriptorPoolSize modelPoolSize = {};
		modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());*/

		//Data to create descriptor pool
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = 0;
        //Maximum number of descriptor sets that can be created from pool
		poolCreateInfo.maxSets = setsCount;
        //Amount of pool sizes being passed
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        //Pool sizes to create pool with
		poolCreateInfo.pPoolSizes = poolSizes.data();

		//Create descriptor pool
		VK_CHECK(vkCreateDescriptorPool(logicalDevice, &poolCreateInfo, nullptr, &mDescriptorPool));
    }

    void VulkanDescriptorPool::destroy(VkDevice logicalDevice)
    {
        vkDestroyDescriptorPool(logicalDevice, mDescriptorPool, nullptr);
    }
}

std::size_t std::hash<fre::VulkanDescriptorPoolKey>::operator()(const fre::VulkanDescriptorPoolKey& key) const
{
	std::size_t seed = 0;
	fre::VkDescriptorPoolSizeHasher poolHasher;
	for(const auto& item : key.mPoolSizes) {
		fre::DPKeyHashCombine(seed, poolHasher(item));
	}
	return seed;
}