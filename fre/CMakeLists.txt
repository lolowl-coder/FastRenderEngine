CPMAddPackage(
	NAME spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.13.0
)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.3.9
    OPTIONS
        "GLFW_BUILD_TESTS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_DOCS OFF"
        "GLFW_INSTALL OFF"
        "GLFW_USE_HYBRID_HPG ON"
)

add_library(FRE STATIC)

# VMA
add_library(VMA INTERFACE)

target_include_directories(VMA INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/VMA/include
)

target_compile_definitions(VMA INTERFACE
    VMA_VULKAN_VERSION=1003000
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)

if (WIN32)
	set(PLATFORM_SOURCES
		include/fre/platform/IGLFWWindow.hpp
		include/fre/platform/IWin32Window.hpp
		src/fre/platform/FileSystemWindows.hpp
		src/fre/platform/FileSystemWindows.cpp
		src/fre/platform/PlatformFactoryWindows.cpp
		src/fre/platform/GLFWWindow.hpp
		src/fre/platform/GLFWWindow.cpp
		src/fre/platform/WindowManagerWindows.hpp
		src/fre/platform/WindowManagerWindows.cpp
	)
endif()

message(WARNING "PLATFORM_SOURCES: " ${PLATFORM_SOURCES})

target_sources(FRE PRIVATE
    include/fre/app/Application.hpp
    include/fre/core/IScene.hpp
	include/fre/core/IWindow.hpp
	include/fre/core/FileSystemBase.hpp
	include/fre/core/Hash.hpp
	include/fre/core/FileSystem.hpp
	include/fre/core/VirtualFileSystem.hpp
	include/fre/core/Log.hpp
	include/fre/core/Pointers.hpp
	include/fre/core/PlatformFactory.hpp
    include/fre/core/Requirement.hpp
    include/fre/core/WindowManager.hpp
    include/fre/engine/Engine.hpp
	include/fre/macros/Class.hpp
	include/fre/macros/Member.hpp
	include/fre/renderer/CommonRendererConfig.hpp
	include/fre/renderer/IGpuImage.hpp
	include/fre/renderer/IGpuImageView.hpp
	include/fre/renderer/IGpuResourceFactory.hpp
	include/fre/renderer/IGraphicsContext.hpp
	include/fre/renderer/IRenderer.hpp
	include/fre/renderer/RendererFactory.hpp
	include/fre/renderer/Renderer.hpp
	include/fre/renderer/ISurface.hpp
	include/fre/renderer/Texture.hpp
	include/fre/renderer/TextureHandle.hpp
	include/fre/renderer/backend/Enums.hpp
	include/fre/renderer/backend/IRenderBackend.hpp
	include/fre/renderer/backend/RenderBackendFactory.hpp
    include/fre/renderer/backend/vulkan/Frame.hpp
    include/fre/renderer/backend/vulkan/IVulkanSurface.hpp
    include/fre/renderer/backend/vulkan/VulkanAllocator.hpp
    include/fre/renderer/backend/vulkan/VulkanContext.hpp
    include/fre/renderer/backend/vulkan/VulkanCommandBuffer.hpp
    include/fre/renderer/backend/vulkan/VulkanCommon.hpp
    include/fre/renderer/backend/vulkan/VulkanCore.hpp
    include/fre/renderer/backend/vulkan/VulkanExtension.hpp
    include/fre/renderer/backend/vulkan/VulkanFence.hpp
    include/fre/renderer/backend/vulkan/VulkanImage.hpp
    include/fre/renderer/backend/vulkan/VulkanImageView.hpp
    include/fre/renderer/backend/vulkan/VulkanQueue.hpp
    include/fre/renderer/backend/vulkan/VulkanRenderBackend.hpp
	include/fre/renderer/backend/vulkan/VulkanSemaphore.hpp
	include/fre/renderer/backend/vulkan/VulkanSurfaceWindows.hpp
    include/fre/renderer/backend/vulkan/VulkanSwapchain.hpp
    src/fre/app/Application.cpp
    src/fre/core/FileSystemBase.cpp
    src/fre/core/Format.cpp
    src/fre/core/Log.cpp
	src/fre/core/Requirement.cpp
	src/fre/core/VirtualFileSystem.cpp
	src/fre/engine/Engine.cpp
	${PLATFORM_SOURCES}
    src/fre/renderer/IGpuImage.cpp
    src/fre/renderer/IGpuImageView.cpp
    src/fre/renderer/RendererFactory.cpp
    src/fre/renderer/Renderer.cpp
    src/fre/renderer/TextureManager.cpp
    src/fre/renderer/backend/RenderBackendFactory.cpp
    src/fre/renderer/backend/vulkan/VmaUsage.cpp
    src/fre/renderer/backend/vulkan/VulkanAllocator.cpp
    src/fre/renderer/backend/vulkan/VulkanContext.cpp
    src/fre/renderer/backend/vulkan/VulkanCore.cpp
    src/fre/renderer/backend/vulkan/VulkanDispatch.cpp
    src/fre/renderer/backend/vulkan/VulkanExtension.cpp
    src/fre/renderer/backend/vulkan/VulkanImage.cpp
    src/fre/renderer/backend/vulkan/VulkanImageView.cpp
    src/fre/renderer/backend/vulkan/VulkanRenderBackend.cpp
	src/fre/renderer/backend/vulkan/VulkanSurface.cpp
    src/fre/renderer/backend/vulkan/VulkanSwapchain.cpp
)

target_include_directories(FRE
    PUBLIC include
	PRIVATE src
)

find_package(Vulkan REQUIRED)

target_link_libraries(FRE
    PUBLIC
		Vulkan::Vulkan
		volk
		spdlog::spdlog
	PRIVATE
		glfw
		VMA
)

# Vulkan-HPP
target_compile_definitions(FRE PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_EXCEPTIONS
	VK_NO_PROTOTYPES
	IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)

if (WIN32)
	add_compile_definitions(NOMINMAX)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(FRE PRIVATE
		VK_USE_PLATFORM_WIN32_KHR
	)
endif()