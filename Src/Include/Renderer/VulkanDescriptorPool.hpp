#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>

inline bool operator==(const VkDescriptorPoolSize& lhs, const VkDescriptorPoolSize& rhs)
{
    return lhs.type == rhs.type &&
        lhs.descriptorCount == rhs.descriptorCount;
}

namespace fre
{
    struct VulkanDescriptorPoolKey
    {
        std::vector<VkDescriptorPoolSize> mPoolSizes;

        bool operator == (const VulkanDescriptorPoolKey& other) const
        {
            return mPoolSizes == other.mPoolSizes;
        }
    };

    struct VulkanDescriptorPool
    {
        void create(
            VkDevice logicalDevice,
            uint32_t count,
            //it's possible to create pool of multiple inputs.
            //e. g. color and depth attachments in one pool.
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        void destroy(VkDevice logicalDevice);

        VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    };

    inline void DPKeyHashCombine(std::size_t& seed, std::size_t value)
    {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    struct VkDescriptorPoolSizeHasher
    {
        std::size_t operator()(const VkDescriptorPoolSize& obj) const
        {
            std::size_t h1 = std::hash<uint32_t>{}(static_cast<uint32_t>(obj.type));
            std::size_t h2 = std::hash<uint32_t>{}(obj.descriptorCount);
            std::size_t combined = h1;
            DPKeyHashCombine(combined, h2);
            return combined;
        }
    };
}

namespace std
{
    template <>
    struct hash<fre::VulkanDescriptorPoolKey>
    {
        std::size_t operator()(const fre::VulkanDescriptorPoolKey& key) const;
    };
}