#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Pointers.hpp"
#include "Renderer/VulkanDescriptorPool.hpp"
#include "Renderer/VulkanDescriptorSet.hpp"
#include "Renderer/VulkanDescriptorSetLayout.hpp"
#include "Image.hpp"

#include <map>
#include <vector>
#include <string>

#include <mutex>

namespace fre
{
	class ThreadPool;

	class VulkanTextureManager
	{
	public:
		using LoadImageCallback = std::function<void(const int imageIndex, const int imagesCount)>;
		using TextureCallback = std::function<void(const VulkanTexturePtr& texture)>;
		using TextureInfoCallback = std::function<void(const VulkanTextureInfoPtr& textureInfo)>;

		VulkanTextureManager(const MainDevice& mainDevice)
			: mMainDevice(mainDevice)
		{
		}

		void create();
		void destroy();
		int getImageIdByFilename(const std::string& fileName) const;
		bool isImageCreated(const std::string& fileName) const;
		uint32_t getImagesCount() const;
		uint32_t createTextureInfo(
			const VkSamplerAddressMode addressMode,
			const VkImageTiling tiling,
			const VkImageUsageFlags usageFlags,
			const VkMemoryPropertyFlags memoryFlags,
			const VkImageLayout layout,
			Image& image,
			const uint32_t mipLevelCount);
		VulkanTextureInfoPtr getTextureInfo(const uint32_t id);
		uint32_t createTexture(
			int8_t transferQueueFamilyId,
			int8_t graphicsQueueFamilyId,
			const VkQueue queue,
			const VkCommandPool commandPool,
			const VulkanTextureInfoPtr& info);
		VulkanTexturePtr getTexture(uint32_t id);
        void loadImages(const LoadImageCallback& callback, ThreadPool& threadPool);
		void uploadData(
			int8_t transferQueueFamilyId,
			int8_t graphicsQueueFamilyId,
			const VkQueue queue,
			const VkCommandPool commandPool,
			VulkanTexturePtr& texture,
			const VulkanTextureInfoPtr& info,
			const uint32_t mipLevels);
		void updateTextureImage(
			int8_t transferFamilyId,
			int8_t graphicsFamilyId,
			VkQueue queue,
			VkCommandPool commandPool,
			const VulkanTextureInfoPtr& info);
		VkDeviceMemory getTextureMemory(uint32_t index);
		bool isTextureInfoCreated(uint32_t index);
		void destroyTexture(uint32_t id);
		void forEachTextureInfo(const TextureInfoCallback& callback)
		{
            for(const auto& ti : mTextureInfos)
            {
				if(callback != nullptr)
				{
					callback(ti.second);
				}
            }
		};
		void forEachTexture(const TextureCallback& callback)
		{
            for(const auto& tex : mTextures)
            {
				if(callback != nullptr)
				{
					callback(tex.second);
				}
            }
		};
		
	private:
		MainDevice mMainDevice;
		std::map<uint32_t, VulkanTextureInfoPtr> mTextureInfos;
		std::map<uint32_t, VulkanTexturePtr> mTextures;
		uint32_t mDefaultTextureId = 0;
		std::mutex mMutex;
	};
}