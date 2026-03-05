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
        bool initialize(const RendererConfig& config) override;
        void shutdown() override;

        void beginFrame() override;
        void endFrame() override;

        void waitIdle() override;
    private:
        bool selectQueueFamilies();
        bool createDebugMessenger();
		void evaluateFeatures();

    private:
        bool mEnableValidation = false;
        bool mHeadless = false;

        // Core
        vk::Instance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        // Queues
        uint32_t mGraphicsQueueFamily = UINT32_MAX;
        uint32_t mComputeQueueFamily = UINT32_MAX;
        uint32_t mTransferQueueFamily = UINT32_MAX;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mTransferQueue;

        FileSystemPtr mFS;
        VirtualFileSystemPtr mVFS;

        vk::DebugUtilsMessengerEXT mDebugMessenger;
		RendererConfig mConfig;
    };
}