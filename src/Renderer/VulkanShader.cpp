#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <stdexcept>

namespace fre
{
    void VulkanShader::create(VkDevice logicalDevice, const std::string& path, VkShaderStageFlagBits shaderStage)
    {
        auto shaderCode = readFile(path);
        mShaderModule = createShaderModule(logicalDevice, shaderCode);
        mShaderStage = shaderStage;
    }

    VkShaderModule VulkanShader::createShaderModule(VkDevice logicalDevice, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	void VulkanShader::destroy(VkDevice logicalDevice)
	{
		vkDestroyShaderModule(logicalDevice, mShaderModule, nullptr);
	}
}