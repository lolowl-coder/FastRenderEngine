#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Pointers.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"

#include <map>
#include <vector>
#include <string>

#include <mutex>

namespace fre
{
	struct MainDevice;
	class ThreadPool;

	struct VulkanTextureManager
	{
		using LoadImageCallback = std::function<void(const int imageIndex, const int imagesCount)>;

		void create(VkDevice logicalDevice);
		void destroy(VkDevice logicalDevice);
		int getImageIdByFilename(const std::string& fileName) const;
		bool isImageCreated(const std::string& fileName) const;
		uint32_t getImagesCount() const;
		VulkanTextureInfoPtr createTextureInfo(
			const VkFormat format,
			const VkSamplerAddressMode addressMode,
			const VkImageTiling tiling,
			const VkImageUsageFlags usageFlags,
			const VkMemoryPropertyFlags memoryFlags,
			const VkImageLayout layout,
			const bool isExternal,
			Image& image);
		uint32_t createTexture(
			const MainDevice& mainDevice,
			int8_t transferQueueFamilyId,
			int8_t graphicsQueueFamilyId,
			const VkQueue queue,
			const VkCommandPool commandPool,
			const VulkanTextureInfoPtr& info);
		VulkanTexturePtr getTexture(uint32_t id);
        void loadImages(const LoadImageCallback& callback, ThreadPool& threadPool);
		void uploadData(const MainDevice& mainDevice,
			int8_t transferQueueFamilyId,
			int8_t graphicsQueueFamilyId,
			const VkQueue queue,
			const VkCommandPool commandPool,
			VulkanTexturePtr& texture,
			const VulkanTextureInfoPtr& info);
		void updateTextureImage(
			const MainDevice& mainDevice,
			int8_t transferFamilyId,
			int8_t graphicsFamilyId,
			VkQueue queue,
			VkCommandPool commandPool,
			const VulkanTextureInfoPtr& info);
		VkDeviceMemory getTextureMemory(uint32_t index);
		bool isTextureInfoCreated(uint32_t index);
		void destroyTexture(VkDevice logicalDevice, uint32_t id);
		
	private:
		std::map<uint32_t, VulkanTextureInfoPtr> mTextureInfos;
		std::map<uint32_t, VulkanTexturePtr> mTextures;
		uint32_t mDefaultTextureId = 0;
		std::mutex mMutex;
	};
}