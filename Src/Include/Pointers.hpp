#pragma once

#include <memory>

namespace fre
{
    struct VulkanBuffer;
    struct VulkanDescriptor;
    struct VulkanDescriptorPool;
    struct VulkanDescriptorSet;
    struct VulkanDescriptorSetLayout;
    struct VulkanTextureInfo;
    struct VulkanTexture;
    struct Mesh;
    struct MeshModel;

    using VulkanBufferPtr = std::shared_ptr<VulkanBuffer>;
    using VulkanDescriptorPtr = std::shared_ptr<VulkanDescriptor>;
    using VulkanDescriptorPoolPtr = std::shared_ptr<VulkanDescriptorPool>;
    using VulkanDescriptorSetPtr = std::shared_ptr<VulkanDescriptorSet>;
    using VulkanDescriptorSetLayoutPtr = std::shared_ptr<VulkanDescriptorSetLayout>;
    using VulkanTextureInfoPtr = std::shared_ptr<VulkanTextureInfo>;
    using VulkanTexturePtr = std::shared_ptr<VulkanTexture>;
    using MeshPtr = std::shared_ptr<Mesh>;
    using MeshModelPtr = std::shared_ptr<MeshModel>;
}