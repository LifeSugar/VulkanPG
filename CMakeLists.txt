cmake_minimum_required(VERSION 3.20)
project(VulkanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_CMAKE_ROOT)
find_path(SPIRV_CROSS_INCLUDE_DIR spirv_cross.hpp
    PATHS "${VULKAN_SDK_CMAKE_ROOT}/Include/spirv_cross"
    NO_DEFAULT_PATH
)
find_library(SPIRV_CROSS_CORE_RELEASE spirv-cross-core
    PATHS "${VULKAN_SDK_CMAKE_ROOT}/Lib"
    NO_DEFAULT_PATH
)
find_library(SPIRV_CROSS_CORE_DEBUG spirv-cross-cored
    PATHS "${VULKAN_SDK_CMAKE_ROOT}/Lib"
    NO_DEFAULT_PATH
)
if(NOT SPIRV_CROSS_INCLUDE_DIR OR NOT SPIRV_CROSS_CORE_RELEASE)
    message(FATAL_ERROR "SPIRV-Cross core was not found in VULKAN_SDK")
endif()
add_library(spirv-cross-core STATIC IMPORTED)
set_target_properties(spirv-cross-core PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SPIRV_CROSS_INCLUDE_DIR}"
    IMPORTED_LOCATION "${SPIRV_CROSS_CORE_RELEASE}"
)
if(SPIRV_CROSS_CORE_DEBUG)
    set_target_properties(spirv-cross-core PROPERTIES
        IMPORTED_LOCATION_DEBUG "${SPIRV_CROSS_CORE_DEBUG}"
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
)

# 3. 链接所有库
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
    assimp::assimp
    asset-core
    imgui
)

if(WIN32)
    target_link_libraries(VulkanApp PRIVATE windowscodecs ole32)
endif()
