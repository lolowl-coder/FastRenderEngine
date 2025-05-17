#pragma once

#include <memory>

namespace fre
{
    struct VulkanBuffer;
    struct VulkanDescriptorPool;
    struct VulkanDescriptorSet;
    struct VulkanDescriptorSetLayout;
    struct Mesh;
    struct MeshModel;

    using VulkanBufferPtr = std::shared_ptr<VulkanBuffer>;
    using VulkanDescriptorPoolPtr = std::shared_ptr<VulkanDescriptorPool>;
    using VulkanDescriptorSetPtr = std::shared_ptr<VulkanDescriptorSet>;
    using VulkanDescriptorSetLayoutPtr = std::shared_ptr<VulkanDescriptorSetLayout>;
    using MeshPtr = std::shared_ptr<Mesh>;
    using MeshModelPtr = std::shared_ptr<MeshModel>;
}