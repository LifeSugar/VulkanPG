#include "Editor/EditorApp.h"

namespace VkRenderer
{

void EditorApp::run()
{
    app_.run(makeRunConfig(), editorLayer_);
}

void EditorApp::runRenderTest()
{
    app_.runRenderTest(makeRunConfig(), editorLayer_);
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
    return config;
}

} // namespace VkRenderer
