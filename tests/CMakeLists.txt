add_library(rubia_smoke_tests STATIC
    AppSmokeTests.cpp
    AppSmokeTests.hpp
)
target_include_directories(rubia_smoke_tests PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(rubia_smoke_tests
    PUBLIC
        Rubia::Editor
        rubia_project_options
    PRIVATE
        imgui
        KTX::ktx
)

# The editor keeps its existing command-line smoke-test entry points.
target_link_libraries(VulkanApp PRIVATE rubia_smoke_tests)

if(BUILD_TESTING)
    add_test(NAME asset-smoke-test
        COMMAND VulkanApp
            "${PROJECT_SOURCE_DIR}/Assets/shaders/triangle.vert.spv"
    )
    set_tests_properties(asset-smoke-test PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:VulkanApp>"
    )
endif()
