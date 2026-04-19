#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/core/Requirement.hpp"
#include "fre/renderer/CommonRendererConfig.hpp"
#include "fre/renderer/IGpuResourceFactory.hpp"
#include "fre/renderer/backend/IRenderBackend.hpp"
#include "fre/renderer/backend/vulkan/Frame.hpp"
#include "fre/renderer/backend/vulkan/VulkanAllocator.hpp"
#include "fre/renderer/backend/vulkan/VulkanQueue.hpp"

#include <map>

namespace fre
{
    class VulkanContext;
    class VulkanCommandBuffer;
    class VulkanQueue;

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

		virtual void drawFrame(IScene* scene) override;

        virtual GpuImagePtr createGpuImage(const IGpuImage::Desc& desc) override;
        virtual GpuImageViewPtr createGpuImageView(IGpuImage* image, const IGpuImageView::Desc& desc) override;
    private:
        bool createLogicalDevice();
        bool createAllocator();
		bool createQueues();
		bool createPools();
        bool selectPhysicalDevice();
        bool selectQueueFamilies();
        bool createDebugMessenger();
        bool createFrames();
		bool evaluateFeatures(const vk::PhysicalDevice& physicalDevice);
        bool evaluateExtensions(const vk::PhysicalDevice& physicalDevice);
		int scorePhysicalDevice(const vk::PhysicalDevice& device);
        VulkanContext& getContext();
        void recordCommands(VulkanCommandBuffer& cmdBuff, const uint32_t imageIndex, IScene* scene);
        void submit(const Frame& frame);
        void present(const Frame& frame);

    private:
        bool mHeadless = false;

        // Core
        vk::PhysicalDevice mPhysicalDevice;
        vk::Device mDevice;
        // Queues
        struct QueueFamilies
        {
            QueueSelection mGraphics;
            QueueSelection mCompute;
            QueueSelection mTransfer;
            QueueSelection mPresent;
        } mQueueFamilySelection;
		std::map<uint32_t, uint32_t> mQueueCounts;
        VulkanQueuePtr mGraphicsQueue;
        VulkanQueuePtr mComputeQueue;
        VulkanQueuePtr mTransferQueue;
        VulkanQueuePtr mPresentQueue;
        VulkanCommandPoolPtr mGraphicsCommandPool;
        VulkanCommandPoolPtr mComputeCommandPool;
        VulkanCommandPoolPtr mTransferCommandPool;
        VulkanCommandPoolPtr mPresentCommandPool;

        vk::DebugUtilsMessengerEXT mDebugMessenger;
		CommonRendererConfig mCommonConfig;
		Desc mDesc;
        IVulkanSurface* mVkSurface = nullptr;
        VulkanAllocatorPtr mAllocator;
        VulkanSwapchainPtr mSwapchain;

        // size = frames in flight
        std::vector<Frame> mFrames;
        uint32_t mCurrentFrame = 0;
        const uint32_t mFramesInFlight = 3;
    };
}