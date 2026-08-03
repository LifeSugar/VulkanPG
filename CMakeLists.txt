cmake_minimum_required(VERSION 3.20)
project(VulkanProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
file(GLOB_RECURSE VULKAN_APP_SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE VULKAN_APP_HEAD_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/head/*.h"
    "${CMAKE_SOURCE_DIR}/head/*.hpp"
)

set(COMMON_SOURCES ${VULKAN_APP_SRC_FILES} ${VULKAN_APP_HEAD_FILES})

# Remove platform-specific entry points from the common set.
list(FILTER COMMON_SOURCES EXCLUDE REGEX "src/main\\.cpp$")
list(FILTER COMMON_SOURCES EXCLUDE REGEX "src/android_main\\.cpp$")

# ---------------------------------------------------------------------------
# Android build
# ---------------------------------------------------------------------------
if(ANDROID)
    # android_native_app_glue static library
    add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue)

    # Dependencies for Android: glm is header-only, assimp needs to be
    # cross-compiled. Install via vcpkg or provide paths manually.
    #   vcpkg install glm:arm64-android assimp:arm64-android
    find_package(glm CONFIG QUIET)
    find_package(assimp CONFIG QUIET)

    if(NOT glm_FOUND)
        message(WARNING "glm not found via find_package. "
            "Set glm_DIR to the glm install path.")
    endif()
    if(NOT assimp_FOUND)
        message(WARNING "assimp not found via find_package. "
            "Set assimp_DIR to the assimp install path. "
            "Build with: vcpkg install assimp:arm64-android")
    endif()

    # Exclude GLFW-dependent sources from Android.
    list(FILTER COMMON_SOURCES EXCLUDE REGEX "src/Window\\.cpp$")
    list(FILTER COMMON_SOURCES EXCLUDE REGEX "head/Window\\.h$")

    add_library(VulkanApp SHARED
        ${COMMON_SOURCES}
        src/android_main.cpp
    )

    target_include_directories(VulkanApp PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/head
        ${CMAKE_SOURCE_DIR}/src
    )

    if(DEFINED Vulkan_INCLUDE_DIRS)
        target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
    endif()

    target_compile_definitions(VulkanApp PRIVATE
        VK_USE_PLATFORM_ANDROID_KHR
        PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        GLM_FORCE_DEPTH_ZERO_TO_ONE
    )

    target_link_libraries(VulkanApp PRIVATE
        Vulkan::Vulkan
        android
        native_app_glue
        log
    )
    if(glm_FOUND)
        target_link_libraries(VulkanApp PRIVATE glm::glm)
    endif()
    if(assimp_FOUND)
        target_link_libraries(VulkanApp PRIVATE assimp::assimp)
    endif()

# ---------------------------------------------------------------------------
# Desktop build
# ---------------------------------------------------------------------------
else()
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(assimp CONFIG REQUIRED)

    add_executable(VulkanApp
        ${COMMON_SOURCES}
        src/main.cpp
    )

    target_include_directories(VulkanApp PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/head
        ${CMAKE_SOURCE_DIR}/src
    )

    target_compile_options(VulkanApp PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    )

    if(DEFINED Vulkan_INCLUDE_DIRS)
        target_include_directories(VulkanApp PRIVATE ${Vulkan_INCLUDE_DIRS})
    endif()

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

    target_compile_definitions(VulkanApp PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        GLM_FORCE_DEPTH_ZERO_TO_ONE
    )

    target_link_libraries(VulkanApp PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        assimp::assimp
    )
endif()
