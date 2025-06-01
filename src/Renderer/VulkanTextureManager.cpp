#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Renderer/VulkanTexture.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Log.hpp"
#include "Mutexes.hpp"
#include "ThreadPool.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <stdexcept>

using namespace glm;

namespace fre
{
    std::mutex gImagesMutex;

	void VulkanTextureManager::create(VkDevice logicalDevice)
	{
		LOG_INFO("Create texture manager");

		LOG_INFO("Texture manager created");
	}

	void VulkanTextureManager::destroy(VkDevice logicalDevice)
	{
		for (size_t i = 0; i < mTextures.size(); i++)
		{
			destroyTexture(logicalDevice, i);
		}
	}

	int VulkanTextureManager::getImageIdByFilename(const std::string& fileName) const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		int result = -1;
		for(const auto& ti : mTextureInfos)
		{
			if(endsWith(ti.second->mImage.mFileName, fileName))
			{
				result = ti.second->mId;
				break;
			}
		}

		return result;
	}

	bool VulkanTextureManager::isImageCreated(const std::string& fileName) const
	{
		bool result = getImageIdByFilename(fileName) != -1;

		return result;
	}

	uint32_t VulkanTextureManager::getImagesCount() const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		return static_cast<uint32_t>(mTextures.size());
	}
	
	uint32_t VulkanTextureManager::createTextureInfo(
		const VkFormat format,
		const VkSamplerAddressMode addressMode,
        const VkImageTiling tiling,
        const VkImageUsageFlags usageFlags,
        const VkMemoryPropertyFlags memoryFlags,
		const VkImageLayout layout,
		const bool isExternal,
		Image& image)
	{
		uint32_t id = mTextureInfos.size();
		VulkanTextureInfoPtr result = mTextureInfos[id];
		result->mId = id;
		result->mAddressMode = addressMode;
		result->mTiling = tiling;
		result->mUsageFlags = usageFlags;
		result->mMemoryFlags = memoryFlags;
		result->mLayout = layout;
		result->mImage = image;

		return id;
	}

	VulkanTextureInfoPtr VulkanTextureManager::getTextureInfo(const uint32_t id)
	{
		VulkanTextureInfoPtr result;
		auto found = mTextureInfos.find(id);
		if(found != mTextureInfos.end())
		{
			result = found->second;
		}

		return result;
	}

	uint32_t VulkanTextureManager::createTexture(
		const MainDevice& mainDevice,
		int8_t transferQueueFamilyId,
		int8_t graphicsQueueFamilyId,
		const VkQueue queue,
		const VkCommandPool commandPool,
		const VulkanTextureInfoPtr& info)
	{
		uint32_t id = mTextures.size();
		VulkanTexturePtr result = mTextures[id];
		result->mId = id;

		if(info->mImage.mDimension.x > 0 && info->mImage.mDimension.y > 0)
		{
			//Create image to hold final texture
			if(info->mImage.mIsExternal)
			{
				result->mImage = fre::createExternalImage(
					mainDevice, info->mImage.mDimension.x, info->mImage.mDimension.y,
					info->mImage.mFormat, info->mTiling,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, getDefaultMemHandleType(), &result->mImageMemory, result->mActualSize);
			}
			else
			{
				result->mImage = fre::createImage(mainDevice, info->mImage.mDimension.x, info->mImage.mDimension.y,
					info->mImage.mFormat, info->mTiling,
					info->mUsageFlags,
					info->mMemoryFlags,
					&result->mImageMemory,
					result->mActualSize);
			}
			result->mImageView = createImageView(mainDevice.logicalDevice,
				result->mImage, info->mImage.mFormat,
				VK_IMAGE_ASPECT_COLOR_BIT);

			//Is texture data passed?
			if(info->mImage.mData != nullptr)
			{
				uploadData(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId,
					queue, commandPool, result, info);
			}
		}

		return id;
	}

	VulkanTexturePtr VulkanTextureManager::getTexture(const uint32_t id)
	{
		VulkanTexturePtr result;
		auto& found = mTextures.find(id);

		if(found != mTextures.end())
		{
			result = found->second;
		}

		return result;
	}

	void VulkanTextureManager::loadImages(const LoadImageCallback& callback, ThreadPool& threadPool)
	{
		uint32_t cnt = mTextureInfos.size();
		for(uint32_t i = 0; i < cnt; i++)
		{
			//load default texture in main thread
			if(i == 0)
			{
				auto& image = mTextureInfos[i]->mImage;
				image.load();
			}
			else
			{
				threadPool.enqueue
				(
					[this, i, cnt, callback]
					{
						std::lock_guard<std::mutex> lock(mMutex);

						auto& image = mTextureInfos[i]->mImage;
						image.load();

						if(callback != nullptr)
						{
							callback(i, cnt);
						}

					}
				);
			}
		}
	}

    void VulkanTextureManager::uploadData(
		const MainDevice& mainDevice,
		int8_t transferQueueFamilyId,
		int8_t graphicsQueueFamilyId,
		const VkQueue queue,
		const VkCommandPool commandPool,
		VulkanTexturePtr& texture,
		const VulkanTextureInfoPtr& info)
	{
		//Create staging buffer to hold loaded data, ready to copy to device
		VulkanBuffer imageStagingBuffer;
		VkDeviceMemory imageStagingBufferMemory;
		createBuffer(mainDevice, info->mImage.mDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0,
			&imageStagingBuffer.mBuffer, nullptr, &imageStagingBuffer.mBufferMemory);

		//Copy image data to staging buffer
		void* data;
		VK_CHECK(vkMapMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory, 0, info->mImage.mDataSize, 0, &data));
		//Fill texture with zeoes if filename is not provided
		if(info->mImage.mData != nullptr)
		{
			memcpy(data, info->mImage.mData, static_cast<size_t>(info->mImage.mDataSize));
		}
		else
		{
			memset(data, 0, static_cast<size_t>(info->mImage.mDataSize));
		}
		vkUnmapMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory);

		//Copy data to image
		//Transition image to be DST for copy operation
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texture->mImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyImageBuffer(mainDevice.logicalDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool,
			imageStagingBuffer.mBuffer, texture->mImage, info->mImage.mDimension.x, info->mImage.mDimension.y);

		//Transition image to be shader readable for shader usage
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texture->mImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//Destroy staging buffers
		vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer.mBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory, nullptr);

		info->mImage.destroy();
	}

	void VulkanTextureManager::updateTextureImage(
		const MainDevice& mainDevice,
		int8_t transferQueueFamilyId,
		int8_t graphicsQueueFamilyId,
		VkQueue queue,
		VkCommandPool commandPool,
		const VulkanTextureInfoPtr& info)
	{
		if(mTextureInfos[info->mId]->mImage.mDimension != info->mImage.mDimension)
		{
			destroyTexture(mainDevice.logicalDevice, info->mId);
			createTexture(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool, info);
		}
		else
		{
            uploadData(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId,
                queue, commandPool, mTextures[info->mId], info);
		}
	}

	VkDeviceMemory VulkanTextureManager::getTextureMemory(uint32_t index)
	{
		VkDeviceMemory result = VK_NULL_HANDLE;

		const auto& found = mTextures.find(index);
		if(found != mTextures.end())
		{
			result = found->second->mImageMemory;
		}

		return result;
	}

	bool VulkanTextureManager::isTextureInfoCreated(uint32_t index)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		return mTextureInfos.find(index) != mTextureInfos.end();
	}

	void VulkanTextureManager::destroyTexture(VkDevice logicalDevice, uint32_t id)
	{
		vkDestroyImageView(logicalDevice, mTextures[id]->mImageView, nullptr);
		vkDestroyImage(logicalDevice, mTextures[id]->mImage, nullptr);
		vkFreeMemory(logicalDevice, mTextures[id]->mImageMemory, nullptr);
	}
}