#include "Renderer/VulkanDescriptorSetLayout.hpp"

#include <stdexcept>
#include <cassert>

namespace fre
{
    void VulkanDescriptorSetLayout::create(
        VkDevice logicalDevice,
        const VulkanDescriptorSetLayoutInfo& key)
    {
		assert
		(
			key.mBindings.size() == key.mDescriptorCount.size() &&
			key.mDescriptorCount.size() == key.mDescriptorTypes.size()
		);

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.resize(key.mDescriptorTypes.size());
        for(uint32_t i = 0; i < key.mDescriptorTypes.size(); i++)
		{
            layoutBindings[i].binding = i;	//Binding point in shader (designated by binding number in shader)
            layoutBindings[i].descriptorType = key.mDescriptorTypes[i];	//Type of descriptor (uniform, dynamic uniform, image sample, etc.)
            layoutBindings[i].descriptorCount = key.mDescriptorCount[i];	//Number of descriptors for binding
            layoutBindings[i].stageFlags = key.mStageFlags[i];	//Shader stage we bind to
            layoutBindings[i].pImmutableSamplers = nullptr;	//Immutability by specifying the layout
        }

		//Create descriptor set layout with given bindings
		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutCreateInfo.pBindings = layoutBindings.data();

		//Create descriptor set layout
		VkResult result = vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &mDescriptorSetLayout);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Descriptor Set Layout!");
		}
    }

    void VulkanDescriptorSetLayout::destroy(VkDevice logicalDevice)
    {
        vkDestroyDescriptorSetLayout(logicalDevice, mDescriptorSetLayout, nullptr);
    }
}