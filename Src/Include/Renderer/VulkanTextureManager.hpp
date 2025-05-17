#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Image.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"

#include <map>
#include <vector>
#include <string>

namespace fre
{
	struct MainDevice;

	struct VulkanTextureManager
	{
		void create(VkDevice logicalDevice);
		void destroy(VkDevice logicalDevice);
		int getImageIdByFilename(const std::string& fileName) const;
		bool isImageCreated(const std::string& fileName) const;
		Image& createImage(const std::string& fileName, bool isExternal);
		Image& createImage(bool isExternal);
		Image* getImage(uint32_t id);
		uint32_t getImagesCount() const;
		VulkanImage createImageGPU(
			const MainDevice& mainDevice,
			VkQueue queue,
			VkCommandPool commandPool,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usageFlags,
			VkMemoryPropertyFlags memoryFlags,
			VkImageLayout layout,
			glm::vec2 size);
		void createTextureImage(
			const MainDevice& mainDevice,
			int8_t transferFamilyId,
			int8_t graphicsFamilyId,
			VkQueue queue,
			VkCommandPool commandPool,
			Image& image);
		void updateTextureImage(
			const MainDevice& mainDevice,
			int8_t transferFamilyId,
			int8_t graphicsFamilyId,
			VkQueue queue,
			VkCommandPool commandPool,
			uint32_t imageId,
			Image& image);
		VkDeviceMemory getTextureMemory(uint32_t index);
		void createTexture(const MainDevice& mainDevice, int8_t transferFamilyId,
			int8_t graphicsQueueFamilyId, VkQueue queue,
			VkCommandPool commandPool, Image& image);
		void destroyTexture(VkDevice logicalDevice, uint32_t id);
		void createTextureDescriptorSet(VkDevice logicalDevice, VkImageView imageView,
			uint32_t index);
		bool isImageAvailable(uint32_t index) const;
		const VulkanDescriptorSet& getDescriptorSet(const MainDevice& MainDevice,
			uint8_t transferQueueFamilyId, uint8_t graphicsQueueFamilyId, VkQueue queue,
			VkCommandPool commandPool, uint32_t descriptorSetIndex);

	private:
		void createTextureSampler(VkDevice logicalDevice, VkSamplerAddressMode addressMode);

	private:
		VulkanDescriptorPool mSamplerDescriptorPool;
		std::map<uint32_t, VkImage> mTextureImages;
		std::map<uint32_t, VkDeviceMemory> mTextureImageMemory;
		std::map<uint32_t, VkImageView> mTextureImageViews;
		std::map<uint32_t, Image> mImages;
		uint32_t mDefaultTextureId = 0;

	public:
		VulkanDescriptorSetLayout mSamplerDescriptorSetLayout;
		std::map<uint32_t, VulkanDescriptorSet> mSamplerDescriptorSets;
		VkSampler mTextureSampler = VK_NULL_HANDLE;
	};
}