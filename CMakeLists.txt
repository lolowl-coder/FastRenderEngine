cmake_minimum_required(VERSION 3.26.0)

project(
    "FRE"
    VERSION 1.0.0
    LANGUAGES C CXX)
	
# Global CMake variables are set here
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #Generate json for each translation unit

# External libraries integration tool
option(USE_CPM "Whether to use CPM." ON)

option(ENABLE_WARNINGS "Enable to add warnings to a target." ON)
option(ENABLE_WARNINGS_AS_ERRORS "Enable to treat warnings as errors." OFF)

option(ENABLE_CLANG_TIDY "Enable to add clang tidy." OFF)

option(ENABLE_SANITIZE_ADDR "Enable address sanitize." OFF)
option(ENABLE_SANITIZE_UNDEF "Enable undefined sanitize." OFF)
option(ENABLE_SANITIZE_LEAK "Enable leak sanitize (Gcc/Clang only)." OFF)
option(ENABLE_SANITIZE_THREAD "Enable thread sanitize (Gcc/Clang only)." OFF)

option(ENABLE_LTO "Enable to add Link Time Optimization." ON)

# Project/Library Names
set(LIBRARY_NAME "fre")
set(EXECUTABLE_NAME "Basic")

# CMAKE MODULES
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(ConfigSafeGuards)
include(AddGitSubmodule)
include(Tools)

if(ENABLE_WARNINGS)
    include(Warnings)
endif()

if(ENABLE_SANITIZE_ADDR OR ENABLE_SANITIZE_UNDEF)
    include(Sanitizer)
    add_sanitizer_flags()
endif()

if(ENABLE_LTO)
    include(LTO)
endif()

if(USE_CPM)
    message(STATUS "Using CPM")
    include(CPM)
    #examples:
	# cpmaddpackage("gh:nlohmann/json@3.11.2")
    # cpmaddpackage("gh:catchorg/Catch2@2.13.9")
    CPMAddPackage(
        NAME glfw
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.3.9
        OPTIONS
            "GLFW_BUILD_TESTS Off"
            "GLFW_BUILD_EXAMPLES Off"
            "GLFW_BUILD_DOCS Off"
            "GLFW_INSTALL On"
            "GLFW_USE_HYBRID_HPG On"
    )
    set(glfw_INCLUDE_DIRS ${GLFW_SOURCE_DIR}/include)
    #message(WARNING "GLFW dirs: " ${glfw_INCLUDE_DIRS})
    CPMAddPackage(
        NAME assimp
        GIT_REPOSITORY https://github.com/assimp/assimp
        GIT_TAG v5.3.1  # Specify the version you want
    )
    set(assimp_INCLUDE_DIRS)
    list(APPEND assimp_INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/_deps/assimp-src/include"
        "${CMAKE_BINARY_DIR}/_deps/assimp-build/include"
    )
    #message(WARNING "Assimp include dirs: ${assimp_INCLUDE_DIRS}")

endif()

# Use FetchContent for single-header libs

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG master  # You can use a specific release tag if needed
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG master  # You can use a specific release tag if needed
)
FetchContent_MakeAvailable(stb_image)

find_package(Vulkan REQUIRED)

# SUB DIRECTORIES

add_subdirectory(configured)
#add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(samples)

# Copy resources

# Copy shaders
#get_target_property(EXE_LOCATION ${EXECUTABLE_NAME} RUNTIME_OUTPUT_DIRECTORY)
#message(WARNING "Exe location: ${EXE_LOCATION}")
#get_target_property(EXE_DIR ${EXECUTABLE_NAME} RUNTIME_OUTPUT_DIRECTORY)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Shaders DESTINATION ${EXE_DIR})
#
## Copy meshes
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Models DESTINATION ${EXE_DIR})
#
## Copy textures
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Textures DESTINATION ${EXE_DIR})

# INSTALL TARGETS

# Install shaders
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Shaders DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Install meshes
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Models DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Install textures
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Data/Textures DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(
    TARGETS ${EXECUTABLE_NAME}
    EXPORT ${LIBRARY_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(
    TARGETS ${LIBRARY_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)