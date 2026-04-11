#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/backend/IRenderBackend.hpp"
#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"

namespace fre
{
    class VulkanContext;

    class VulkanRenderBackend : public IRenderBackend, public IGpuResourceFactory
    {
    public:
        struct Desc
        {
            struct mFeatures
            {
                DEFINE_REQUIREMENT(dynamicRendering);
                DEFINE_REQUIREMENT(timelineSemaphore);
                DEFINE_REQUIREMENT(descriptorIndexing);
                DEFINE_REQUIREMENT(bufferDeviceAddress);
                DEFINE_REQUIREMENT(synchronization2);
                DEFINE_REQUIREMENT(accelerationStructure);
                DEFINE_REQUIREMENT(rayTracingPipeline);
                DEFINE_REQUIREMENT(rayQuery);
            } mFeatures;

            std::vector<RequirementRequest> mInstanceExtensions =
            {
                { VK_KHR_SURFACE_EXTENSION_NAME, Requirement::Required },
                { VK_KHR_WIN32_SURFACE_EXTENSION_NAME, Requirement::Required },
                { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, Requirement::Optional }
            };

            std::vector<RequirementRequest> mDeviceExtensions =
            {
                { VK_KHR_SWAPCHAIN_EXTENSION_NAME, Requirement::Optional }
            };
        };

        VulkanRenderBackend(const CommonRendererConfig& commonConfig);
        ~VulkanRenderBackend();
        bool initialize() override;
        void shutdown() override;
        void waitIdle() override;

        virtual GpuImagePtr createGpuImage(const IGpuImage::Desc& desc) override;
        virtual GpuImageViewPtr createGpuImageView(IGpuImage* image, const IGpuImageView::Desc& desc) override;
    private:
        bool createLogicalDevice();
        bool createAllocator();
        bool selectPhysicalDevice();
        bool selectQueueFamilies();
        bool createDebugMessenger();
		bool evaluateFeatures(const vk::PhysicalDevice& physicalDevice);
        bool evaluateExtensions(const vk::PhysicalDevice& physicalDevice);
		int scorePhysicalDevice(const vk::PhysicalDevice& device);
        VulkanContext& getContext();

    private:
        bool mHeadless = false;

        // Core
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        // Queues
        struct QueueFamilies
        {
            uint32_t graphics = UINT32_MAX;
            uint32_t compute = UINT32_MAX;
            uint32_t transfer = UINT32_MAX;
            uint32_t present = UINT32_MAX;
        } mQueueFamilies;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mTransferQueue;
        vk::Queue mPresentQueue;

        vk::DebugUtilsMessengerEXT mDebugMessenger;
		CommonRendererConfig mCommonConfig;
		Desc mDesc;
        IVulkanSurface* mVkSurface = nullptr;
        VulkanAllocatorPtr mAllocator;
        VulkanSwapchainPtr mSwapchain;
    };
}