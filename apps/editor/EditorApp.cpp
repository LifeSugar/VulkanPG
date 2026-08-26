#include "EditorApp.hpp"

#include "AppSmokeTests.hpp"

namespace VkRenderer
{

void EditorApp::run()
{
    app_.run(makeRunConfig(), editorLayer_);
}

void EditorApp::runRenderTest()
{
    Test::AppSmokeTests::runRenderTest(
        app_,
        makeRunConfig(),
        editorLayer_);
}

App::RunConfig EditorApp::makeRunConfig()
{
    App::RunConfig config{};
    config.windowWidth = 1600;
    config.windowHeight = 900;
    config.windowTitle = "Vulkan Editor";
    config.enableDocking = true;
    config.imguiIniFilename = "editor_imgui.ini";
    config.outputMode = VulkanRenderer::OutputMode::Editor;
    config.demoContent = DemoContentLoader::CreateInfo{
        "ABeautifulGame_extracted/ABeautifulGame.gltf",
        "shaders/triangle.vert.spv",
        "shaders/triangle.frag.spv",
        "shaders/present.vert.spv",
        "shaders/present.frag.spv",
        "assets",
        DemoContentLoader::TextureImportPolicy{
            KtxPayloadEncoding::Uastc,
            false,
            TextureFormat::BC7UNorm,
            TextureFormat::BC7UNorm,
            TextureFormat::BC5UNorm,
            true}};
    return config;
}

} // namespace VkRenderer
