#include "Renderer/VulkanShader.hpp"
#include "Utilities.hpp"

#include <stdexcept>
#include <filesystem>

namespace fre
{
	bool ShaderMetaData::isValid() const
	{
		return !mDescriptorSetLayouts.empty();
	}

	const std::vector<char> VulkanShader::create(VkDevice logicalDevice, const std::string& path, VkShaderStageFlagBits shaderStage)
	{
		namespace fs = std::filesystem;
		std::vector<char> shaderSource;
		if(fs::exists(path))
		{
			LOG_TRACE("Loading shader: {}", path);

			shaderSource = readFile(path);
			mShaderModule = createShaderModule(logicalDevice, shaderSource);
			mShaderStage = shaderStage;
		}

		return shaderSource;
	}

    VkShaderModule VulkanShader::createShaderModule(VkDevice logicalDevice, const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = code.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule));

		return shaderModule;
	}

	void VulkanShader::destroy(VkDevice logicalDevice)
	{
		if(mShaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(logicalDevice, mShaderModule, nullptr);
			mShaderModule = VK_NULL_HANDLE;
		}
	}
}