#include "AppSmokeTests.hpp"

#include "EditorLayer.hpp"
#include "vulkan/UploadContext.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace rubia::test
{

void AppSmokeTests::runStartupTest()
{
    // Keep log capture alive until App has joined its worker and drained uploads.
    editor::EditorLayer gui;
    editor::App app;
    editor::App::RunConfig config{};
    config.outputMode = rhi::vulkan::VulkanRenderer::OutputMode::Editor;
    config.autoLoadDemo = false;
    const auto start = std::chrono::steady_clock::now();
    app.initWindow(config, false);
    app.initVulkan(config);
    app.setupCamera();
    app.initImGui(config);
    app.guiRenderBridge.attach(app.renderer, app.renderAssets);
    gui.attach({app.assetManager, app.scene, app.guiRenderBridge,
        &app.textureImports, &app.contentLoadStatus_});
    const auto draw = [&]
    {
        app.window.pollEvents();
        app.imguiLayer.beginFrame();
        app.drawGui(gui);
        if (app.renderer.renderGui(app.imguiLayer.endFrame()) ==
            rhi::vulkan::VulkanRenderer::RenderResult::NeedsResize)
        {
            app.recreateSwapChain(gui);
        }
    };
    try
    {
        draw();
        const auto firstGui = std::chrono::steady_clock::now();
        if (app.contentLoadFuture_.valid() || app.renderer.sceneReady())
        {
            throw std::runtime_error("empty startup unexpectedly started content loading");
        }
        app.recreateSwapChain(gui);
        draw();

        // Cooperative cancellation also works before the worker gets scheduled.
        app.contentLoadConfig_ = config.demoContent;
        app.startContentLoading();
        app.discardContentLoading();
        if (app.contentLoadFuture_.valid() || app.preparedContent_ ||
            !app.assetManager.textureHandles().empty())
        {
            throw std::runtime_error("CPU cancellation left live content or a worker");
        }
        app.contentLoadStatus_ = {};

        // Valid builtins must not make a missing model fatal to the editor.
        app.contentLoadConfig_.modelPath = "__missing_startup_model__.gltf";
        app.startContentLoading();
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (app.contentLoadStatus_.state == editor::ContentLoadState::Preparing &&
            std::chrono::steady_clock::now() < deadline)
        {
            app.updateContentLoading();
            draw();
        }
        if (app.contentLoadStatus_.state != editor::ContentLoadState::Failed ||
            !app.renderer || !app.scene.nodes().empty())
        {
            throw std::runtime_error("missing model did not leave a usable empty editor");
        }

        app.contentLoadConfig_ = config.demoContent;
        app.startContentLoading();
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
        while (std::chrono::steady_clock::now() < deadline)
        {
            app.updateContentLoading();
            if (app.contentLoadStatus_.state == editor::ContentLoadState::Failed)
            {
                throw std::runtime_error(app.contentLoadStatus_.message);
            }
            draw();
            if (app.contentUploads_ && app.contentUploads_->stagedByteCount() != 0)
            {
                break;
            }
        }
        if (!app.contentUploads_ || app.contentUploads_->stagedByteCount() == 0)
        {
            throw std::runtime_error("upload shutdown test did not submit a batch");
        }
        // Exercise the close path while staging and GPU destinations are owned.
        glfwSetWindowShouldClose(app.window.nativeHandle(), GLFW_TRUE);
        app.mainLoop(gui);
        app.discardContentLoading();
        app.renderer.waitIdle();
        gui.detach();
        app.cleanup();
        if (app.window || app.renderer || app.contentUploads_ ||
            app.contentLoadFuture_.valid())
        {
            throw std::runtime_error("startup shutdown left application resources alive");
        }
        std::clog << "[Startup] First GUI submission: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(firstGui - start).count()
            << " ms; empty resize, CPU cancellation, missing model and upload shutdown passed\n";
    }
    catch (...)
    {
        app.discardContentLoading();
        app.renderer.waitIdle();
        gui.detach();
        app.cleanup();
        throw;
    }
}

} // namespace rubia::test
