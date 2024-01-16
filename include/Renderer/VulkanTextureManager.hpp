#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"

#include <vector>
#include <string>

namespace fre
{
    struct MainDevice;

    struct VulkanTextureManager
    {
        void create(VkDevice logicalDevice);
        void destroy(VkDevice logicalDevice);
		int createTextureImage(const MainDevice& mainDevice, VkQueue queue,
            VkCommandPool commandPool, std::string fileName);
		int createTexture(const MainDevice& mainDevice, VkQueue queue,
			VkCommandPool commandPool, std::string fileName);
        int createTextureDescriptorSet(VkDevice logicalDevice, VkImageView imageView);

    private:
        void createTextureSampler(VkDevice logicalDevice);

    public:
		VulkanDescriptorPool mSamplerDescriptorPool;
		VulkanDescriptorSetLayout mSamplerDescriptorSetLayout;
		std::vector<VulkanDescriptorSet> mSamplerDescriptorSets;
		VkSampler mTextureSampler;
		std::vector<VkImage> mTextureImages;
		std::vector<VkDeviceMemory> mTextureImageMemory;
		std::vector<VkImageView> mTextureImageViews;
    };
}