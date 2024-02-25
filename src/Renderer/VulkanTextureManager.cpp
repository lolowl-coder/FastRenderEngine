#include "Renderer/VulkanBufferManager.hpp"
#include "Renderer/VulkanTextureManager.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Mutexes.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <stdexcept>

namespace fre
{
	void VulkanTextureManager::create(VkDevice logicalDevice)
	{
		mSamplerDescriptorPool.create(
			logicalDevice,
			0,
			MAX_OBJECTS,
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER});

		mSamplerDescriptorSetLayout.create(
			logicalDevice,
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
			VK_SHADER_STAGE_FRAGMENT_BIT);

		createTextureSampler(logicalDevice);
	}

    void VulkanTextureManager::createTextureSampler(VkDevice logicalDevice)
    {
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; //For border clamp only
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;	//normalized: 0-1 space, unnormalized: 0-imageSize
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;	//Level of details bias for mip level
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = 16.0f;	//Anisotropy sample level

		VkResult result = vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &mTextureSampler);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Texture Sampler!");
		}
	}

	void VulkanTextureManager::destroy(VkDevice logicalDevice)
	{
		vkDestroySampler(logicalDevice, mTextureSampler, nullptr);

		mSamplerDescriptorSetLayout.destroy(logicalDevice);
		mSamplerDescriptorPool.destroy(logicalDevice);

		for (size_t i = 0; i < mTextureImages.size(); i++)
		{
			vkDestroyImageView(logicalDevice, mTextureImageViews[i], nullptr);
			vkDestroyImage(logicalDevice, mTextureImages[i], nullptr);
			vkFreeMemory(logicalDevice, mTextureImageMemory[i], nullptr);
		}
	}

	void VulkanTextureManager::loadImage(
		std::string fileName, uint32_t id)
	{
		//Load image file
		Image image;
		image.mId = id;
		image.load(fileName);
		
		{
			std::lock_guard<std::mutex> lock(gImagesMutex);
			mImages[id] = image;
			
        	std::cout << "image loaded: " << fileName << ", id: " << id << std::endl;
		}
	}

	uint32_t VulkanTextureManager::getImagesCount() const
	{
		return static_cast<uint32_t>(mImages.size());
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
		VkBuffer imageStagingBuffer;
		VkDeviceMemory imageStagingBufferMemory;
		createBuffer(mainDevice, image.mDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&imageStagingBuffer, &imageStagingBufferMemory);

		//Copy image data to staging buffer
		void* data;
		vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, image.mDataSize, 0, &data);
		memcpy(data, image.mData, static_cast<size_t>(image.mDataSize));
		vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);

		//Create image to hold final texture
		VkImage texImage;
		VkDeviceMemory texImageMemory;
		texImage = createImage(mainDevice, image.mDimension.x, image.mDimension.y,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

		//Copy data to image
		//Transition image to be DST for copy operation
		transitionImageLayout(mainDevice.logicalDevice, queue,
			commandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyImageBuffer(mainDevice.logicalDevice, transferQueueFamilyId, graphicsQueueFamilyId, queue, commandPool,
			imageStagingBuffer, texImage, image.mDimension.x, image.mDimension.y);

		//Transition image to be shader readable for shader usage
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		mTextureImages[image.mId] = texImage;
		mTextureImageMemory[image.mId] = texImageMemory;

		//Destroy staging buffers
		vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

		image.destroy();
	}

	void VulkanTextureManager::createTexture(const MainDevice& mainDevice,
		int8_t transferQueueFamilyId, int8_t graphicsQueueFamilyId, VkQueue queue,
		VkCommandPool commandPool, Image& image)
	{
		createTextureImage(mainDevice, transferQueueFamilyId,
			graphicsQueueFamilyId, queue, commandPool, image);

		VkImageView imageView = createImageView(mainDevice.logicalDevice,
			mTextureImages[image.mId], VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT);
		mTextureImageViews[image.mId] = imageView;

		//Create Texture Descriptor
		createTextureDescriptorSet(mainDevice.logicalDevice, imageView, image.mId);
	}

	void VulkanTextureManager::createTextureDescriptorSet(VkDevice logicalDevice,
		VkImageView imageView, uint32_t index)
	{
		VulkanDescriptorSet descriptorSet;
		descriptorSet.allocate(
			logicalDevice, mSamplerDescriptorPool.mDescriptorPool,
			mSamplerDescriptorSetLayout.mDescriptorSetLayout);
		descriptorSet.update(logicalDevice,
			{imageView}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			mTextureSampler);

		mSamplerDescriptorSets[index] = descriptorSet;
	}

	bool VulkanTextureManager::isImageAvailable(uint32_t index) const
	{
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