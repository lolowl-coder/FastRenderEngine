#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace fre
{
    struct VulkanCommandBuffers
    {
        void create(uint32_t count, VkCommandPool graphicsCommandPool, VkDevice logicalDevice);
        void begin(uint32_t currentImageIndex);
        void recordCommands(uint32_t currentImage);
        void end(uint32_t currentImageIndex);
        VkCommandBuffer get(uint32_t currentImageIndex);

    private:
		  std::vector<VkCommandBuffer> mCommandBuffers;
    };
}