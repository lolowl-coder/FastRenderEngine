#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace fre
{
    struct VulkanVertexAttribute
    {
        VkFormat mFormat = VK_FORMAT_UNDEFINED;
        uint32_t mOffset = 0;
    };

    //Describes stages and all inputs for them (vertex input, attachments to read, etc.)
    struct VulkanShader
    {
        void create(VkDevice logicalDevice, const std::string& path, VkShaderStageFlagBits shaderStage);
        void destroy(VkDevice logicalDevice);

        VkShaderStageFlagBits mShaderStage;
        VkShaderModule mShaderModule = VK_NULL_HANDLE;
    private:
        VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);
    };
}