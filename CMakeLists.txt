cmake_minimum_required(VERSION 3.10)
project(VulkanProject)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(VulkanApp main.cpp)

# Keep runtime assets next to the executable so relative paths like
# shaders/triangle.vert.spv work regardless of build folder layout.
add_custom_command(TARGET VulkanApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:VulkanApp>/shaders"
    COMMENT "Copying shaders to output directory"
)

# 3. 链接所有库
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
)