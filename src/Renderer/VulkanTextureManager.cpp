#include "Renderer/VulkanTextureManager.hpp"
#include "Renderer/VulkanImage.hpp"
#include "Utilities.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>

namespace fre
{
	void VulkanTextureManager::create(VkDevice logicalDevice)
	{
		mSamplerDescriptorPool.create(
			logicalDevice,
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

	stbi_uc* loatTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
	{
		//Number of channels image uses
		int channels;

		//Load pixel data of image
		std::string fileLoc = "Textures/" + fileName;
		stbi_uc* image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);

		if (!image)
		{
			throw std::runtime_error("Failed to load a texture file! (" + fileName + ")");
		}

		*imageSize = *width * *height * 4;

		return image;
	}

	int VulkanTextureManager::createTextureImage(
		const MainDevice& mainDevice,
		VkQueue queue,
		VkCommandPool commandPool,
		std::string fileName)
	{
		//Load image file
		int width, height;
		VkDeviceSize imageSize;
		stbi_uc* imageData = loatTextureFile(fileName, &width, &height, &imageSize);

		//Create staging buffer to hold loaded data, ready to copy to device
		VkBuffer imageStagingBuffer;
		VkDeviceMemory imageStagingBufferMemory;
		createBuffer(mainDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&imageStagingBuffer, &imageStagingBufferMemory);

		//Copy image data to staging buffer
		void* data;
		vkMapMemory(mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, imageData, static_cast<size_t>(imageSize));
		vkUnmapMemory(mainDevice.logicalDevice, imageStagingBufferMemory);
		stbi_image_free(imageData);

		//Create image to hold final texture
		VkImage texImage;
		VkDeviceMemory texImageMemory;
		texImage = createImage(mainDevice, width, height,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

		//Copy data to image
		//Transition image to be DST for copy operation
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyImageBuffer(mainDevice.logicalDevice, queue, commandPool, imageStagingBuffer, texImage, width, height);

		//Transition image to be shader readable for shader usage
		transitionImageLayout(mainDevice.logicalDevice, queue, commandPool, texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		mTextureImages.push_back(texImage);
		mTextureImageMemory.push_back(texImageMemory);

		//Destroy staging buffers
		vkDestroyBuffer(mainDevice.logicalDevice, imageStagingBuffer, nullptr);
		vkFreeMemory(mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

		//Return image index
		return mTextureImages.size() - 1;
	}

	int VulkanTextureManager::createTexture(const MainDevice& mainDevice, VkQueue queue,
			VkCommandPool commandPool, std::string fileName)
	{
		int textureImageLoc = createTextureImage(mainDevice, queue, commandPool, fileName);

		VkImageView imageView = createImageView(mainDevice.logicalDevice,
			mTextureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT);
		mTextureImageViews.push_back(imageView);

		//Create Texture Descriptor
		int descriptorLoc = createTextureDescriptorSet(mainDevice.logicalDevice, imageView);

		return descriptorLoc;
	}

	int VulkanTextureManager::createTextureDescriptorSet(VkDevice logicalDevice, VkImageView imageView)
	{
		VulkanDescriptorSet textureDescriptorSet;
		textureDescriptorSet.allocate(
			logicalDevice, mSamplerDescriptorPool.mDescriptorPool,
			mSamplerDescriptorSetLayout.mDescriptorSetLayout);
		textureDescriptorSet.update(logicalDevice,
			{imageView}, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			mTextureSampler);

		mSamplerDescriptorSets.push_back(textureDescriptorSet);

		return mSamplerDescriptorSets.size() - 1;
	}
}