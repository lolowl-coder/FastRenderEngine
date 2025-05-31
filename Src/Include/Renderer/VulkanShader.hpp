#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Renderer/Callbacks.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <functional>

namespace fre
{
    struct Camera;
    struct Light;
    struct Material;

    struct VulkanVertexAttribute
    {
        VkFormat mFormat = VK_FORMAT_UNDEFINED;
        uint32_t mOffset = 0;
    };

    struct ShaderMetaData
    {
        //Callback to pass variables to shader
        PushConstantCallback mPushConstantsCallback = nullptr;
        //Callback to pass data like textures or buffers to shader
        BindDescriptorSetsCallback mBindDescriptorSetsCallback = nullptr;

        //Almost all fields are used to create pipeline

        //Vertex attributes
        std::vector<VulkanVertexAttribute> mVertexAttributes;
        //Descriptor set layout
        std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
        //Push constant ranges used to create a pipeline and to push constants to shader
        std::vector<VkPushConstantRange> mPushConstantRanges;
        //Vertex size
        uint32_t mVertexSize = 0u;
        //Subpass index
        uint32_t mSubPassIndex = 0u;
        //Geomery topology
        VkPrimitiveTopology mTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        //Color blending attachments count
        uint32_t mAttachmentsCount = 1u;
        //Line width if shader is used for lines rendering
        float mLineWidth = 0.0f;
        //Depth test enabled flag
        bool mDepthTestEnabled = false;
        //Cull mode
        VkCullModeFlags mCullMode = VK_CULL_MODE_BACK_BIT;
        //Metadata considered valid if it has descriptor set layouts
        bool isValid() const;
    };

    using ShaderMetaDatum = std::vector<ShaderMetaData>;

    class ShaderMetaDataProvider
    {
    public:
        virtual ShaderMetaDatum getShaderMetaData(const std::string& shaderFileName) = 0;
    };

    //Describes stages and all inputs for them (vertex input, attachments to read, etc.)
    struct VulkanShader
    {
        const std::vector<char> create(VkDevice logicalDevice, const std::string& path, VkShaderStageFlagBits shaderStage);
        void destroy(VkDevice logicalDevice);

        VkShaderStageFlagBits mShaderStage = static_cast<VkShaderStageFlagBits>(0);
        VkShaderModule mShaderModule = VK_NULL_HANDLE;
    private:
        VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);
    };
}