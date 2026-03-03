#pragma once

#include <vulkan/vulkan.hpp>
#include "fre/IRenderer.hpp"
#include "fre/core/Pointers.hpp"
#include "fre/core/VirtualFileSystem.hpp"
#include "fre/core/IFileSystem.hpp"

namespace fre
{
    class VulkanRenderer : public IRenderer
    {
    public:
		VulkanRenderer();
        bool initialize(const RendererDesc& desc) override;
        void shutdown() override;

        void beginFrame() override;
        void endFrame() override;

        void waitIdle() override;
    private:
        bool selectQueueFamilies();

    private:
        bool m_enableValidation = false;
        bool m_headless = false;

        // Core
        vk::Instance m_instance;
        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_device;
        // Queues
        uint32_t m_graphicsQueueFamily = UINT32_MAX;
        uint32_t m_computeQueueFamily = UINT32_MAX;
        uint32_t m_transferQueueFamily = UINT32_MAX;
        vk::Queue m_graphicsQueue;
        vk::Queue m_computeQueue;
        vk::Queue m_transferQueue;

        FileSystemPtr mFS;
        VirtualFileSystemPtr mVFS;
    };
}