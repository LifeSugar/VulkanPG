add_library(lv_importers STATIC)
add_library(LearnVulkan::Importers ALIAS lv_importers)

target_include_directories(lv_importers
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${Stb_INCLUDE_DIR}"
)
target_link_libraries(lv_importers
    PUBLIC
        LearnVulkan::Engine
        lv_project_options
    PRIVATE
        assimp::assimp
        KTX::ktx
        spirv-cross-core
)

add_subdirectory(gltf)
add_subdirectory(shader)
add_subdirectory(texture)
