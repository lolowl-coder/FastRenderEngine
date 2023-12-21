# Sources and Headers
set(LIBRARY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MeshModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utilities.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Renderer/VulkanRenderer.cpp")
set(LIBRARY_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Engine.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Mesh.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/MeshModel.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Utilities.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Hash/Hash.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Macros/Class.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Macros/Member.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/Renderer/VulkanRenderer.hpp")
set(LIBRARY_INCLUDES_PUBLIC
    "./"
    "${CMAKE_BINARY_DIR}/configured_files/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${glm_SOURCE_DIR}"
    "${stb_image_SOURCE_DIR}"
    "${glfw_INCLUDE_DIRS}"
    "${Vulkan_INCLUDE_DIRS}"
    "${assimp_INCLUDE_DIRS}"
    )

set(LIBRARY_INCLUDES_PRIVATE
    )

# Vulkan SDK

# MyLib Library
add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})
target_include_directories(${LIBRARY_NAME} PUBLIC
    ${LIBRARY_INCLUDES_PUBLIC}
    )
target_include_directories(${LIBRARY_NAME} PRIVATE
    ${LIBRARY_INCLUDES_PRIVATE}
    )

#target_link_libraries(
#    ${LIBRARY_NAME}
#    PRIVATE nlohmann_json::nlohmann_json
#            fmt::fmt
#            spdlog::spdlog
#            cxxopts::cxxopts)

target_link_libraries(${LIBRARY_NAME} PRIVATE
    glfw
    ${Vulkan_LIBRARIES}
    assimp
    )

if(${ENABLE_WARNINGS})
    target_set_warnings(
        TARGET
        ${LIBRARY_NAME}
        ENABLE
        ${ENABLE_WARNINGS}
        AS_ERRORS
        ${ENABLE_WARNINGS_AS_ERRORS})
endif()

if(${ENABLE_LTO})
    target_enable_lto(
        TARGET
        ${LIBRARY_NAME}
        ENABLE
        ON)
endif()

if(${ENABLE_CLANG_TIDY})
    add_clang_tidy_to_target(${LIBRARY_NAME})
endif()
