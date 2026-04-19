#pragma once

#include <memory>

namespace fre
{
	class Engine;
	class IGraphicsContext;
	class IFileSystem;
	class VirtualFileSystem;
	class IWindow;
	class IWindowManager;
	class IGpuImage;
	class IGpuImageView;
	class Texture;
	class IRenderBackend;
	class IRenderer;
	class IScene;
	class ISurface;
	class IVulkanSurface;
	class VulkanAllocator;
	class VulkanCommandPool;
	class VulkanSurfaceWindows;
	class VulkanImage;
	class VulkanImageView;
	class VulkanQueue;
	class VulkanSwapchain;

	using EnginePtr = std::unique_ptr<Engine>;
	using GraphicsContextPtr = std::unique_ptr<IGraphicsContext>;
	using FileSystemPtr = std::unique_ptr<IFileSystem>;
	using VirtualFileSystemPtr = std::unique_ptr<VirtualFileSystem>;
	using WindowPtr = std::unique_ptr<IWindow>;
	using WindowManagerPtr = std::unique_ptr<IWindowManager>;
	using GpuImagePtr = std::unique_ptr<IGpuImage>;
	using GpuImageViewPtr = std::unique_ptr<IGpuImageView>;
	using TexturePtr = std::unique_ptr<Texture>;
	using RenderBackendPtr = std::unique_ptr<IRenderBackend>;
	using RendererPtr = std::unique_ptr<IRenderer>;
	using ScenePtr = std::unique_ptr<IScene>;
	using SurfacePtr = std::unique_ptr<ISurface>;
	using VulkanAllocatorPtr = std::unique_ptr<VulkanAllocator>;
	using VulkanCommandPoolPtr = std::unique_ptr<VulkanCommandPool>;
	using VulkanImagePtr = std::unique_ptr<VulkanImage>;
	using VulkanImageViewPtr = std::unique_ptr<VulkanImageView>;
	using VulkanQueuePtr = std::unique_ptr<VulkanQueue>;
	using VulkanSurfacePtr = std::unique_ptr<IVulkanSurface>;
	using VulkanSurfaceWindowsPtr = std::unique_ptr<VulkanSurfaceWindows>;
	using VulkanSwapchainPtr = std::unique_ptr<VulkanSwapchain>;
}