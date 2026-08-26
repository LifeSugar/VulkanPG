find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Ktx CONFIG REQUIRED)

add_library(VulkanMemoryAllocator INTERFACE)
target_include_directories(VulkanMemoryAllocator INTERFACE
    "${PROJECT_SOURCE_DIR}/third_party/vma/include"
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

# Vulkan SDK package metadata is not consistently relocatable, so locate the
# SPIRV-Cross library relative to the SDK while accepting both common casings.
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
    set_property(TARGET spirv-cross-core APPEND PROPERTY
        INTERFACE_LINK_OPTIONS "$<$<CONFIG:Debug>:/IGNORE:4099>"
    )
endif()

set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/third_party/imgui")
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
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)
