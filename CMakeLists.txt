cmake_minimum_required(VERSION 3.10)
project(VulkanProject)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(VulkanApp main.cpp)

# 3. 链接所有库
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
)