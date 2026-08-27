add_library(rubia_importers STATIC)
add_library(Rubia::Importers ALIAS rubia_importers)

target_include_directories(rubia_importers
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${Stb_INCLUDE_DIR}"
)
target_link_libraries(rubia_importers
    PUBLIC
        Rubia::Engine
        rubia_project_options
    PRIVATE
        assimp::assimp
        KTX::ktx
        spirv-cross-core
)

add_subdirectory(gltf)
add_subdirectory(shader)
add_subdirectory(texture)
