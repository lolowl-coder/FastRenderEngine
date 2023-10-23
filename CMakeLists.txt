cmake_minimum_required(VERSION 3.27.7)

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
set(LIBRARIES
	#assimp
	#tiny_gltf
	#stb_image
	)
set(EXECUTABLE_NAME "main")

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

else(USE_CPM)
    message(STATUS "Using CPM")
    include(CPM)
    #examples:
	# cpmaddpackage("gh:nlohmann/json@3.11.2")
    # cpmaddpackage("gh:catchorg/Catch2@2.13.9")
endif()

# SUB DIRECTORIES

add_subdirectory(configured)
add_subdirectory(external)
#add_subdirectory(src)

# INSTALL TARGETS

install(
    TARGETS ${EXECUTABLE_NAME}
    EXPORT ${LIBRARIES}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(
    TARGETS ${LIBRARIES}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
