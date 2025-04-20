#include "Renderer/VulkanDescriptorSetLayout.hpp"

#include <stdexcept>
#include <cassert>

namespace fre
{
    void VulkanDescriptorSetLayout::create(
        VkDevice logicalDevice,
        const std::vector<VkDescriptorType>& descriptorTypes,
        const std::vector<uint32_t>& stageFlags)
    {
		assert(descriptorTypes.size() == stageFlags.size());

        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.resize(descriptorTypes.size());
		assert(descriptorTypes.size() == stageFlags.size());
        for(uint32_t i = 0; i < descriptorTypes.size(); i++)
		{
            layoutBindings[i].binding = i;	//Binding point in shader (designated by binding number in shader)
            layoutBindings[i].descriptorType = descriptorTypes[i];	//Type of descriptor (uniform, dynamic uniform, image sample, etc.)
            layoutBindings[i].descriptorCount = 1;	//Number of descriptors for binding
            layoutBindings[i].stageFlags = stageFlags[i];	//Shader stage we bind to
            layoutBindings[i].pImmutableSamplers = nullptr;	//Immutability by specifying the layout
        }

		//ModelMatrix Binding info
		/*VkDescriptorSetLayoutBinding modelLayoutBinding = {};
		modelLayoutBinding.binding = 1;
		modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelLayoutBinding.descriptorCount = 1;
		modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		modelLayoutBinding.pImmutableSamplers = nullptr;*/

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