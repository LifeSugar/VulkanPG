cmake_minimum_required(VERSION 3.20)
project(VulkanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Ktx CONFIG REQUIRED)

option(VK_RENDERER_USE_VMA "Use Vulkan Memory Allocator in renderer resources" ON)

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party/vma/include"
)
target_link_libraries(VulkanMemoryAllocator INTERFACE Vulkan::Vulkan)

if(DEFINED ENV{VULKAN_SDK})
    file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_CMAKE_ROOT)
elseif(Vulkan_INCLUDE_DIRS)
    list(GET Vulkan_INCLUDE_DIRS 0 VULKAN_INCLUDE_ROOT)
    get_filename_component(VULKAN_SDK_CMAKE_ROOT
        "${VULKAN_INCLUDE_ROOT}" DIRECTORY)
else()
    message(FATAL_ERROR "Unable to determine the Vulkan SDK root")
endif()

# Vulkan SDK package metadata isn't consistently relocatable across platforms,
# so resolve SPIRV-Cross from the SDK root while supporting both directory
# casings used by Windows and macOS distributions.
find_path(SPIRV_CROSS_INCLUDE_DIR spirv_cross.hpp
    PATHS
        "${VULKAN_SDK_CMAKE_ROOT}/Include/spirv_cross"
        "${VULKAN_SDK_CMAKE_ROOT}/include/spirv_cross"
    NO_DEFAULT_PATH
)
find_library(SPIRV_CROSS_CORE_RELEASE spirv-cross-core
    PATHS
        "${VULKAN_SDK_CMAKE_ROOT}/Lib"
        "${VULKAN_SDK_CMAKE_ROOT}/lib"
    NO_DEFAULT_PATH
)
if(MSVC)
    find_library(SPIRV_CROSS_CORE_DEBUG spirv-cross-cored
        PATHS
            "${VULKAN_SDK_CMAKE_ROOT}/Lib"
            "${VULKAN_SDK_CMAKE_ROOT}/lib"
        NO_DEFAULT_PATH
    )
endif()

if(NOT SPIRV_CROSS_INCLUDE_DIR OR NOT SPIRV_CROSS_CORE_RELEASE)
    message(FATAL_ERROR "SPIRV-Cross core was not found in the Vulkan SDK")
endif()

add_library(spirv-cross-core STATIC IMPORTED)
set_target_properties(spirv-cross-core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SPIRV_CROSS_INCLUDE_DIR}"
    IMPORTED_LOCATION "${SPIRV_CROSS_CORE_RELEASE}"
)

if(MSVC AND SPIRV_CROSS_CORE_DEBUG)
    set_target_properties(spirv-cross-core PROPERTIES
        IMPORTED_LOCATION_DEBUG "${SPIRV_CROSS_CORE_DEBUG}"
    )
    # The SDK's Debug archive is ABI-compatible with an MSVC Debug consumer,
    # but its referenced compiler PDB is not distributed. Suppress only that
    # missing third-party debug-symbol warning for consumers of this target.
    set_property(TARGET spirv-cross-core APPEND PROPERTY
        INTERFACE_LINK_OPTIONS "$<$<CONFIG:Debug>:/IGNORE:4099>"
    )
endif()

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")

add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_compile_options(imgui PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(imgui PUBLIC
    Vulkan::Vulkan
    glfw
)

file(GLOB_RECURSE ASSET_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/Asset/*.cpp"
)
list(APPEND ASSET_CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/Import/SpirvReflection.cpp"
)

add_library(asset-core STATIC ${ASSET_CORE_SOURCES})
target_include_directories(asset-core PUBLIC
    ${CMAKE_SOURCE_DIR}/head
)
target_compile_options(asset-core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)
target_compile_definitions(asset-core PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
target_link_libraries(asset-core PUBLIC
    glm::glm
    spirv-cross-core
)

set(VULKAN_APP_SOURCES)

if(EXISTS "${CMAKE_SOURCE_DIR}/src")
    file(GLOB_RECURSE VULKAN_APP_SRC_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cc"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.cxx"
    )
    list(FILTER VULKAN_APP_SRC_FILES EXCLUDE REGEX
        ".*/src/Asset/.*\\.cpp$"
    )
    list(FILTER VULKAN_APP_SRC_FILES EXCLUDE REGEX
        ".*/src/Import/SpirvReflection\\.cpp$"
    )
    list(APPEND VULKAN_APP_SOURCES ${VULKAN_APP_SRC_FILES})
endif()

# 添加head目录的头文件
if(EXISTS "${CMAKE_SOURCE_DIR}/head")
    file(GLOB_RECURSE VULKAN_APP_HEAD_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/head/*.h"
        "${CMAKE_SOURCE_DIR}/head/*.hpp"
    )
    list(APPEND VULKAN_APP_SOURCES ${VULKAN_APP_HEAD_FILES})
endif()

add_executable(VulkanApp
    ${VULKAN_APP_SOURCES}
)

target_include_directories(VulkanApp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/head
    ${CMAKE_SOURCE_DIR}/src
    ${Stb_INCLUDE_DIR}
)

target_compile_options(VulkanApp PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# In some setups IntelliSense does not infer Vulkan headers from transitive
# includes in headers; add them explicitly when provided by find_package.
if(DEFINED Vulkan_INCLUDE_DIRS)
    target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

# Keep runtime assets next to the executable so relative paths like
# Assets/shaders/triangle.vert.spv work regardless of build folder layout.
add_custom_command(TARGET VulkanApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/shaders"
            "$<TARGET_FILE_DIR:VulkanApp>/Assets/shaders"
    COMMENT "Copying shaders to output directory"
)

add_custom_command(TARGET VulkanApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets/Models"
            "$<TARGET_FILE_DIR:VulkanApp>/Assets/Models"
    COMMENT "Copying models to output directory"
)

# 编译期嵌入源码根目录，运行时若相对路径找不到则回退到源码目录
target_compile_definitions(VulkanApp PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    VK_RENDERER_USE_VMA=$<BOOL:${VK_RENDERER_USE_VMA}>
)

# 3. 链接所有库
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
    assimp::assimp
    asset-core
    imgui
    KTX::ktx
)

if(VK_RENDERER_USE_VMA)
    target_link_libraries(VulkanApp PRIVATE VulkanMemoryAllocator)
endif()

# KTX is a shared vcpkg dependency on Windows. Keep its runtime and zstd
# dependency beside the executable so tests and the editor do not depend on
# the caller's PATH.
if(WIN32)
    add_custom_command(TARGET VulkanApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:KTX::ktx>"
                "$<TARGET_FILE_DIR:VulkanApp>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:zstd::libzstd_shared>"
                "$<TARGET_FILE_DIR:VulkanApp>"
        COMMENT "Copying KTX runtime dependencies"
    )
endif()
