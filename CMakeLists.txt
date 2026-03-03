cmake_minimum_required(VERSION 4.2)
project(FRE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(CPM)

add_subdirectory(third_party/volk)
add_subdirectory(fre)
add_subdirectory(app)