#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Log.hpp"
#include "Mutexes.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <stdexcept>
#include <mutex>

using namespace glm;

namespace fre
{
    std::mutex gImagesMutex;

	void VulkanTextureManager::create(VkDevice logicalDevice)
	{
		LOG_INFO("Create texture manager");

		mSamplerDescriptorPool.create(
			logicalDevice,
			0,
			MAX_OBJECTS,
			{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS}}
		);

		mSamplerDescriptorSetLayout.create(
			logicalDevice,
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
			{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT});

		createTextureSampler(logicalDevice, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		LOG_INFO("Texture manager created");
	}

    void VulkanTextureManager::createTextureSampler(VkDevice logicalDevice, VkSamplerAddressMode addressMode)
    {
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = addressMode;
		samplerCreateInfo.addressModeV = addressMode;
		samplerCreateInfo.addressModeW = addressMode;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK; //For border clamp only
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;	//normalized: 0-1 space, unnormalized: 0-imageSize
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;	//Level of details bias for mip level
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16.0f;	//Anisotropy sample level

		VK_CHECK(vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &mTextureSampler));
	}

	void VulkanTextureManager::destroyTexture(VkDevice logicalDevice, uint32_t id)
	{
		vkDestroyImageView(logicalDevice, mTextureImageViews[id], nullptr);
		vkDestroyImage(logicalDevice, mTextureImages[id], nullptr);
		vkFreeMemory(logicalDevice, mTextureImageMemory[id], nullptr);
	}

	void VulkanTextureManager::destroy(VkDevice logicalDevice)
	{
		vkDestroySampler(logicalDevice, mTextureSampler, nullptr);

		mSamplerDescriptorSetLayout.destroy(logicalDevice);
		mSamplerDescriptorPool.destroy(logicalDevice);

		for (size_t i = 0; i < mTextureImages.size(); i++)
		{
			destroyTexture(logicalDevice, i);
		}
	}

	Image& VulkanTextureManager::createImage(const std::string& fileName, bool isExternal)
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		uint32_t id = mImages.size();
		mImages[id] = Image();
		auto& result = mImages[id];
		result.mId = id;
		result.mFileName = fileName;
		result.mIsExternal = isExternal;

		return result;
	};

	Image& VulkanTextureManager::createImage(bool isExternal)
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		uint32_t id = mImages.size();
		mImages[id] = Image();
		auto& result = mImages[id];
		result.mId = id;
		result.mIsExternal = isExternal;

		return result;
	};

	Image* VulkanTextureManager::getImage(uint32_t id)
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		Image* result = nullptr;
		auto found = mImages.find(id);

		if(found != mImages.end())
		{
			result = &found->second;
		}

		return result;
	}

	int VulkanTextureManager::getImageIdByFilename(const std::string& fileName) const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		int result = -1;
		for(const auto& image : mImages)
		{
			if(endsWith(image.second.mFileName, fileName))
				result = image.second.mId;
		}

		return result;
	}

	bool VulkanTextureManager::isImageCreated(const std::string& fileName) const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		bool result = false;
		for(const auto& image : mImages)
		{
			if(endsWith(image.second.mFileName, fileName))
				result = true;
		}

		return result;
	}

	uint32_t VulkanTextureManager::getImagesCount() const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		return static_cast<uint32_t>(mImages.size());
	}
	
	VulkanImage createImageGPU(
		const MainDevice& mainDevice,
		VkQueue queue,
		VkCommandPool commandPool,
		VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryFlags,
		VkImageLayout layout,
		vec2 size)
	{
		VulkanImage result;

		//Create image to hold final texture
		result.mImage = fre::createImage(mainDevice, size.x, size.y,
			format, tiling,
			usageFlags,
			memoryFlags,
			&result.mImageMemory,
			result.mActualSize);
		result.mImageView = createImageView(mainDevice.logicalDevice,
			result.mImage, format,
			VK_IMAGE_ASPECT_COLOR_BIT);

		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, result.mImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, layout);
	}

	void VulkanTextureManager::createTextureImage(
		const MainDevice& mainDevice,
		int8_t transferQueueFamilyId,
		int8_t graphicsQueueFamilyId,
		VkQueue queue,
		VkCommandPool commandPool,
		Image& image)
	{
		//Create staging buffer to hold loaded data, ready to copy to device
		VulkanBuffer imageStagingBuffer;
		VkDeviceMemory imageStagingBufferMemory;
		createBuffer(mainDevice, image.mDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			0,
			&imageStagingBuffer.mBuffer, nullptr, &imageStagingBuffer.mBufferMemory);

		//Copy image data to staging buffer
		void* data;
		VK_CHECK(vkMapMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory, 0, image.mDataSize, 0, &data));
		//Fill texture with zeoes if filename is not provided
		if(image.mData != nullptr)
		{
			memcpy(data, image.mData, static_cast<size_t>(image.mDataSize));
		}
		else
		{
			memset(data, 0, static_cast<size_t>(image.mDataSize));
		}
		vkUnmapMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory);

		//Create image to hold final texture
		VkImage texImage;
		VkDeviceMemory& texImageMemory = mTextureImageMemory[image.mId];
		if(image.mIsExternal)
		{
			texImage = fre::createExternalImage(mainDevice, image.mDimension.x, image.mDimension.y,
				image.mFormat, image.mTiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, getDefaultMemHandleType(), &texImageMemory, image.mActualSize);
		}
		else
		{
			texImage = fre::createImage(mainDevice, image.mDimension.x, image.mDimension.y,
				image.mFormat, image.mTiling,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory, image.mActualSize);
		}

		//Copy data to image
		//Transition image to be DST for copy operation
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyImageBuffer(mainDevice.logicalDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool,
			imageStagingBuffer.mBuffer, texImage, image.mDimension.x, image.mDimension.y);

		//Transition image to be shader readable for shader usage
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		mTextureImages[image.mId] = texImage;

		//Destroy staging buffers
		vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer.mBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, imageStagingBuffer.mBufferMemory, nullptr);

		image.destroy();
	}
	
	void VulkanTextureManager::updateTextureImage(
		const MainDevice& mainDevice,
		int8_t transferQueueFamilyId,
		int8_t graphicsQueueFamilyId,
		VkQueue queue,
		VkCommandPool commandPool,
		uint32_t imageId,
		Image& image)
	{
		if(mImages[imageId].mDimension != image.mDimension)
		{
			destroyTexture(mainDevice.logicalDevice, imageId);
			createTexture(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool, image);
		}
		else
		{
			//Create staging buffer to hold loaded data, ready to copy to device
			VkBuffer imageStagingBuffer;
			uint64_t imageDeviceAddress = 0;
			VkDeviceMemory imageStagingBufferMemory;
			createBuffer(mainDevice, image.mDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
				&imageStagingBuffer, &imageDeviceAddress, &imageStagingBufferMemory);

			//Copy image data to staging buffer
			void* data;
			VK_CHECK(vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, image.mDataSize, 0, &data));
			if(image.mData != nullptr)
			{
				memcpy(data, image.mData, static_cast<size_t>(image.mDataSize));
			}
			else
			{
				memset(data, 0, static_cast<size_t>(image.mDataSize));
			}
			vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);

			//Create image to hold final texture
			VkImage texImage = mTextureImages[imageId];
			VkDeviceMemory texImageMemory = mTextureImageMemory[imageId];

			//Copy data to image
			//Transition image to be DST for copy operation
			transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			copyImageBuffer(mainDevice.logicalDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool,
				imageStagingBuffer, texImage, image.mDimension.x, image.mDimension.y);

			//Transition image to be shader readable for shader usage
			transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			//Destroy staging buffers
			vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
			vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);
			
			image.destroy();
		}

		//Update image
		mImages[imageId] = image;
	}

	VkDeviceMemory VulkanTextureManager::getTextureMemory(uint32_t index)
	{
		VkDeviceMemory result = VK_NULL_HANDLE;

		const auto& found = mTextureImageMemory.find(index);
		if(found != mTextureImageMemory.end())
		{
			result = found->second;
		}

		return result;
	}

	void VulkanTextureManager::createTexture(const MainDevice& mainDevice,
		int8_t transferQueueFamilyId, int8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool commandPool, Image& image)
	{
		createTextureImage(mainDevice, transferQueueFamilyId,
			graphicsQueueFamilyId, queue, commandPool, image);

		VkImageView imageView = createImageView(mainDevice.logicalDevice,
			mTextureImages[image.mId], image.mFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
		mTextureImageViews[image.mId] = imageView;

		//Create Texture Descriptor
		createTextureDescriptorSet(mainDevice.logicalDevice, imageView, image.mId);
	}

	void VulkanTextureManager::createTextureDescriptorSet(VkDevice logicalDevice,
		VkImageView imageView, uint32_t index)
	{
		if(mSamplerDescriptorSets.find(index) == mSamplerDescriptorSets.end())
		{
			VulkanDescriptorSet descriptorSet;
			descriptorSet.allocate(
				logicalDevice, mSamplerDescriptorPool.mDescriptorPool,
				mSamplerDescriptorSetLayout.mDescriptorSetLayout);
			descriptorSet.update(logicalDevice,
				{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{imageView}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
				{mTextureSampler});

			mSamplerDescriptorSets[index] = descriptorSet;
		}
		else
		{
			VulkanDescriptorSet& descriptorSet = mSamplerDescriptorSets[index];
			descriptorSet.update(logicalDevice,
				{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{imageView}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
				{mTextureSampler});
		}
	}

	bool VulkanTextureManager::isImageAvailable(uint32_t index) const
	{
		std::lock_guard<std::mutex> lock(gImagesMutex);
		return mImages.find(index) != mImages.end();
	}

	const VulkanDescriptorSet& VulkanTextureManager::getDescriptorSet(const MainDevice& mainDevice,
		uint8_t transferQueueFamilyId, uint8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool commandPool, uint32_t index)
	{
		const VulkanDescriptorSet* result = nullptr;
		
		if(isImageAvailable(index))
		{
			auto foundIt = mSamplerDescriptorSets.find(index);
			if(foundIt != mSamplerDescriptorSets.end())
			{
				result = &(foundIt->second);
			}
			else
			{
				{
					std::lock_guard<std::mutex> lock(gImagesMutex);
					createTexture(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId,
						queue, commandPool, mImages[index]);
				}
				result = &mSamplerDescriptorSets[index];
			}
		}
		else
		{
			//Return default texture
			result = &getDescriptorSet(mainDevice, transferQueueFamilyId, graphicsQueueFamilyId,
				queue, commandPool, mDefaultTextureId);
		}

		return *result;
	}
}