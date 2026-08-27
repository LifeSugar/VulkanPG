cmake_minimum_required(VERSION 3.23)

project(RubiaEngine LANGUAGES CXX)

option(VK_RENDERER_USE_VMA "Use Vulkan Memory Allocator in renderer resources" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(CTest)

add_library(rubia_project_options INTERFACE)
target_compile_features(rubia_project_options INTERFACE cxx_std_17)
target_compile_options(rubia_project_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

add_subdirectory(engine)
add_subdirectory(importers)
add_subdirectory(renderer)
add_subdirectory(apps)
add_subdirectory(tests)
