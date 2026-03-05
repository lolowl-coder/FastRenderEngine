CPMAddPackage(
	NAME spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.13.0
)

add_library(FRE STATIC)

if (WIN32)
	set(PLATFORM_SOURCES
		src/platform/FileSystemWindows.hpp
		src/platform/FileSystemWindows.cpp
		src/platform/PlatformFactoryWindows.cpp
		src/platform/WindowWindows.hpp
		src/platform/ExternalWindowWindows.hpp
	)
endif()

target_sources(FRE PRIVATE
	include/fre/core/FileSystemBase.hpp
	include/fre/core/IFileSystem.hpp
	include/fre/core/IWindow.hpp
	include/fre/core/VirtualFileSystem.hpp
	include/fre/core/Log.hpp
	include/fre/core/NativeWindowHandle.hpp
	include/fre/core/Pointers.hpp
	include/fre/core/PlatformFactory.hpp
	include/fre/core/Hash.hpp
	include/fre/macros/Class.hpp
	include/fre/macros/Member.hpp
	include/fre/IRenderer.hpp
	include/fre/RendererConfig.hpp
	include/fre/RendererFactory.hpp
    src/core/FileSystemBase.cpp
    src/core/Log.cpp
	src/core/VirtualFileSystem.cpp
	${PLATFORM_SOURCES}
    src/vulkan/VulkanDispatch.cpp
    src/vulkan/VulkanRenderer.hpp
    src/vulkan/VulkanRenderer.cpp
    src/RendererFactory.cpp
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
)

# Vulkan-HPP
target_compile_definitions(FRE PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_EXCEPTIONS
	VK_NO_PROTOTYPES
)