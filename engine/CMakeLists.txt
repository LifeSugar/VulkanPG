add_library(lv_engine STATIC)
add_library(LearnVulkan::Engine ALIAS lv_engine)

target_include_directories(lv_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(lv_engine PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
target_link_libraries(lv_engine
    PUBLIC
        glm::glm
        lv_project_options
)

add_subdirectory(asset)
add_subdirectory(math)
add_subdirectory(render)
add_subdirectory(scene)
