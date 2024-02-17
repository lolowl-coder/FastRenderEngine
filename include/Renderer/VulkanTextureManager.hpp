#pragma once

#define GLFW_INCLUDE_VULKAN
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
    void loadImage(const std::string fileName, uint32_t id);
    uint32_t getImagesCount() const;
    void createTextureImage(
      const MainDevice& mainDevice,
      int8_t transferFamilyId,
		  int8_t graphicsFamilyId,
      VkQueue queue,
      VkCommandPool commandPool,
      Image& image);
    void createTexture(const MainDevice& mainDevice, int8_t transferFamilyId,
		  int8_t graphicsQueueFamilyId, VkQueue queue,
      VkCommandPool commandPool, Image& image);
    void createTextureDescriptorSet(VkDevice logicalDevice, VkImageView imageView,
      uint32_t index);
    bool isImageAvailable(uint32_t index) const;
    const VulkanDescriptorSet& getDescriptorSet(const MainDevice& MainDevice,
      uint8_t transferQueueFamilyId, uint8_t graphicsQueueFamilyId, VkQueue queue,
      VkCommandPool commandPool, uint32_t descriptorSetIndex);

  private:
    void createTextureSampler(VkDevice logicalDevice);

  private:
    VulkanDescriptorPool mSamplerDescriptorPool;
    VkSampler mTextureSampler;
    std::map<uint32_t, VkImage> mTextureImages;
    std::map<uint32_t, VkDeviceMemory> mTextureImageMemory;
    std::map<uint32_t, VkImageView> mTextureImageViews;
    std::map<uint32_t, Image> mImages;
    uint32_t mDefaultTextureId = 0;

  public:
    VulkanDescriptorSetLayout mSamplerDescriptorSetLayout;
    std::map<uint32_t, VulkanDescriptorSet> mSamplerDescriptorSets;
  };
}