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

option(ENABLE_WARNINGS "Enable to add warnings to a target." OFF)
option(ENABLE_WARNINGS_AS_ERRORS "Enable to treat warnings as errors." OFF)

option(ENABLE_CLANG_TIDY "Enable to add clang tidy." OFF)

option(ENABLE_SANITIZE_ADDR "Enable address sanitize." OFF)
option(ENABLE_SANITIZE_UNDEF "Enable undefined sanitize." OFF)
option(ENABLE_SANITIZE_LEAK "Enable leak sanitize (Gcc/Clang only)." OFF)
option(ENABLE_SANITIZE_THREAD "Enable thread sanitize (Gcc/Clang only)." OFF)

if(NOT CMAKE_BUILD_TYPE)
    # Set default build type
    set(CMAKE_BUILD_TYPE Release)
endif()

message(WARNING "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_LTO "Enable to add Link Time Optimization." OFF)
else()
    option(ENABLE_LTO "Enable to add Link Time Optimization." ON)
endif()

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
            "GLFW_BUILD_TESTS OFF"
            "GLFW_BUILD_EXAMPLES OFF"
            "GLFW_BUILD_DOCS OFF"
            "GLFW_INSTALL OFF"
            "GLFW_USE_HYBRID_HPG ON"
    )
    set(glfw_INCLUDE_DIRS ${GLFW_SOURCE_DIR}/include)
    #message(WARNING "GLFW dirs: " ${glfw_INCLUDE_DIRS})

    # Set Assimp options BEFORE adding the subdirectory
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "Disable all importers by default" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "Enable OBJ importer" FORCE)
    #set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "Enable GLTF importer" FORCE)

    # Disable unnecessary features to minimize size
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable samples" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable exporters" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable tools" FORCE)
	set(ASSIMP_INSTALL OFF CACHE BOOL "Disable installation for Assimp" FORCE)

    CPMAddPackage(
        NAME assimp
        GIT_REPOSITORY https://github.com/assimp/assimp
        GIT_TAG v5.3.1  # Specify the version you want
		OPTIONS
			"ASSIMP_BUILD_TESTS OFF"
            "ASSIMP_BUILD_SAMPLES OFF"
            "ASSIMP_NO_EXPORT ON"
            "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
	        "ASSIMP_INSTALL OFF"
    )
    set(assimp_INCLUDE_DIRS)
    list(APPEND assimp_INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/_deps/assimp-src/contrib/rapidjson/include"
        "${CMAKE_BINARY_DIR}/_deps/assimp-src/include"
        "${CMAKE_BINARY_DIR}/_deps/assimp-build/include"
    )
    #message(WARNING "Assimp include dirs: ${assimp_INCLUDE_DIRS}")
	CPMAddPackage(
        NAME spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0  # Specify the version you want
    )

    set(TIFF_INSTALL_DEFAULT OFF CACHE BOOL "Disable installation for tiff" FORCE)

	CPMAddPackage(
        NAME libtiff
        GIT_REPOSITORY https://gitlab.com/libtiff/libtiff
        GIT_TAG v4.6.0  # Specify the version you want
        OPTIONS
            "tiff-tools OFF"
            "tiff-tests OFF"
            "tiff-contrib OFF"
            "tiff-docs OFF"
            "tiff-install OFF"
    )
	set(tiff_INCLUDE_DIRS)
    list(APPEND tiff_INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/_deps/libtiff-src/libtiff"
        "${CMAKE_BINARY_DIR}/_deps/libtiff-build/libtiff"
    )

    CPMAddPackage(
        NAME NVTX
        GITHUB_REPOSITORY NVIDIA/NVTX
        GIT_TAG v3.1.0-c-cpp
        GIT_SHALLOW TRUE
    )
endif()

# Use FetchContent for single-header libs

include(FetchContent)

#set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

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

FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG master  # or a specific version/tag
)
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
#Required by volk
add_definitions(-DVK_NO_PROTOTYPES)
add_definitions(-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES)

FetchContent_MakeAvailable(volk)

find_package(Vulkan REQUIRED)

# Enable testing
include(CTest)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)
# Prevent Google Test from building its own tests
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# SUB DIRECTORIES

add_subdirectory(configured)
#add_subdirectory(External)

function(buildApp appName sources headers libs includes)
    add_executable(${appName} ${sources} ${headers} ${shaders})
	# Shaders
	set (SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Shaders)
	# Get a list of all shaders in the shaders directory
	file(GLOB_RECURSE SHADER_SOURCES "${SHADERS_DIR}/*.vert" "${SHADERS_DIR}/*.frag" "${SHADERS_DIR}/*.comp" "${SHADERS_DIR}/*.rgen" "${SHADERS_DIR}/*.rchit" "${SHADERS_DIR}/*.rmiss")

	# Create a source group for the shaders
	source_group("Shaders" FILES ${SHADER_SOURCES})

	# Create a list to store the output .spv files
	set(SPV_OUTPUT_FILES "")

	# Iterate over each shader source file
	foreach(SHADER ${SHADER_SOURCES})
		# Get the filename and extension
		get_filename_component(SHADER_FILENAME ${SHADER} NAME)
		# Generate the output filename for the .spv file
		set(SPV_FILE "${SHADERS_DIR}/${SHADER_FILENAME}.spv")
		# Add the .spv file to the list of output files
		list(APPEND SPV_OUTPUT_FILES ${SPV_FILE})
		# Add a custom command to compile the shader
		add_custom_command(
			OUTPUT ${SPV_FILE}
            #glslc -std=450 --target-spv=spv1.4 -mfmt=num --target-env=vulkan1.2 -o shader.rchit.spv shader.rchit
			#COMMAND glslc -std=460 --target-spv=spv1.4 -mfmt=num --target-env=vulkan1.2 -o ${SPV_FILE} ${SHADER}
		    COMMAND glslc --target-spv=spv1.4 -o ${SPV_FILE} ${SHADER}
			#COMMAND glslc -o ${SPV_FILE} ${SHADER}
			DEPENDS ${SHADER}
		)
	endforeach()

	# Create a custom target that depends on the shader compilation
	add_custom_target(CompileShaders DEPENDS ${SPV_OUTPUT_FILES})

	# Link the custom target to your main target
	add_dependencies(${appName} CompileShaders)
	
	if(DEFINED includes)
		target_include_directories(${appName} PRIVATE ${includeDirs})
	endif()
    
    target_link_libraries(
        ${appName}
        PRIVATE
		${libs}
		)

    if (MSVC)
        target_compile_options(${appName} PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP>)
    endif()

    if(${ENABLE_WARNINGS})
        target_set_warnings(
            TARGET
            ${appName}
            ENABLE
            ${ENABLE_WARNINGS}
            AS_ERRORS
            ${ENABLE_WARNINGS_AS_ERRORS})
    endif()

    if(${ENABLE_LTO})
        target_enable_lto(
            TARGET
            ${appName}
            ENABLE
            ON)
    endif()

    if(${ENABLE_CLANG_TIDY})
        add_clang_tidy_to_target(${appName})
    endif()
endfunction()

function(buildLib libName sources headers libsPrivate libsPublic includesPrivate includesPublic)
	add_library(${libName} STATIC ${sources} ${headers})
	target_link_libraries(${libName}
	PRIVATE
		${libsPrivate}
	PUBLIC
		${libsPublic}
    )

	if(${ENABLE_WARNINGS})
		target_set_warnings(
			TARGET
			${libName}
			ENABLE
			${ENABLE_WARNINGS}
			AS_ERRORS
			${ENABLE_WARNINGS_AS_ERRORS})
	endif()

	if(${ENABLE_LTO})
		target_enable_lto(
			TARGET
			${libName}
			ENABLE
			ON)
	endif()

	if(${ENABLE_CLANG_TIDY})
		add_clang_tidy_to_target(${libName})
	endif()

	target_include_directories(${libName}
		PUBLIC
		${includesPublic}
		PRIVATE
		${includesPrivate}
		)
endfunction()

function(installApp appName)
    install(
        TARGETS ${appName}
        #EXPORT ${LIBRARY_NAME}
        #ARCHIVE DESTINATION lib
        #LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
	)

    #install(
    #    TARGETS ${LIBRARY_NAME}
    #    ARCHIVE DESTINATION lib
    #    LIBRARY DESTINATION lib)
endfunction(installApp)

add_subdirectory(Src)
add_subdirectory(Samples)

message(WARNING "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(WARNING "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")