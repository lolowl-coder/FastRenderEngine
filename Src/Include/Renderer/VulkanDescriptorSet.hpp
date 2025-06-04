#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include "Pointers.hpp"

#include <vector>
#include <memory>
#include <functional>

namespace fre
{
    struct MainDevice;

    //We need a descriptor set per inputs combination.
    //Thus key is hash of descriptors handles.
    struct VulkanDescriptorSetKey
    {
        uint32_t mShaderId = std::numeric_limits<uint32_t>::max();
        uint32_t mDPId = std::numeric_limits<uint32_t>::max();
        uint32_t mDSLId = std::numeric_limits<uint32_t>::max();
        uint32_t mMeshId = std::numeric_limits<uint32_t>::max();

        bool operator==(const VulkanDescriptorSetKey& other) const
        {
            return
                mShaderId == other.mShaderId &&
                mDPId == other.mDPId &&
                mDSLId == other.mDSLId &&
                mMeshId == other.mMeshId;
        }
    };

    struct VulkanDescriptorSet
    {
        void allocate(
            const VkDevice logicalDevice,
            const VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout descriptorSetLayout);
        void update(VkDevice logicalDevice, const std::vector<VulkanDescriptorPtr>& descriptors);

        VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

        bool operator==(const VulkanDescriptorSet& other) const
        {
            return
                mDescriptorSet == other.mDescriptorSet &&
                mDescriptorPool == other.mDescriptorPool &&
                mDescriptorSetLayout == other.mDescriptorSetLayout;
        }
    };
}

namespace std
{
    template <>
    struct hash<fre::VulkanDescriptorSetKey>
    {
        std::size_t operator()(const fre::VulkanDescriptorSetKey& key) const;
    };
}