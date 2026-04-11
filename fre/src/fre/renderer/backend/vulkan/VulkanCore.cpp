#include "fre/renderer/backend/vulkan/VulkanCore.hpp"

namespace fre
{
    void vkCheck(vk::Result result)
    {
        if(result != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan error: " + vk::to_string(result));
    }
}