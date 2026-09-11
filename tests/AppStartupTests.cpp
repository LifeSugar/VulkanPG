#include "AppSmokeTests.hpp"

#include "EditorLayer.hpp"
#include "render/SceneResourcePreparation.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace rubia::test
{

void AppSmokeTests::runStartupTest()
{
    using render::ScenePreparationState;
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
            const auto status = app.renderer.scenePreparationStatus();
            if (status.submitted > status.completed)
            {
                break;
            }
        }
        const auto uploading = app.renderer.scenePreparationStatus();
        if (uploading.submitted <= uploading.completed)
        {
            throw std::runtime_error("upload shutdown test did not submit a batch");
        }
        // Keep the CPU source to exercise cancellation and restart independently
        // of the App worker. The backend must not clear an existing operation.
        auto prepared = app.preparedContent_;
        bool duplicateRejected = false;
        try { app.renderer.beginScenePreparation(app.renderAssets, {}); }
        catch (const std::logic_error&) { duplicateRejected = true; }
        if (!duplicateRejected ||
            app.renderer.scenePreparationStatus().submitted != uploading.submitted)
        {
            throw std::runtime_error("duplicate preparation replaced an in-flight operation");
        }
        app.discardContentLoading();
        if (app.renderer.scenePreparationStatus().state != ScenePreparationState::Cancelled ||
            app.renderAssets.materialDescriptorSetLayout() != VK_NULL_HANDLE ||
            app.renderer.sceneReady() || !app.assetManager.textureHandles().empty())
        {
            throw std::runtime_error("upload cancellation left partial scene resources");
        }
        draw();

        app.renderer.beginScenePreparation(app.renderAssets, {});
        if (app.renderer.scenePreparationStatus().state != ScenePreparationState::Failed ||
            app.renderer.scenePreparationStatus().error.empty() || !app.renderer)
        {
            throw std::runtime_error("invalid request did not return a recoverable failure");
        }

        const auto makeRequest = [&]
        {
            render::SceneResourceRequest request;
            request.assets = std::shared_ptr<const asset::AssetManager>(prepared, &prepared->assets);
            request.models = {prepared->content.model};
            request.materialTemplate = prepared->content.materialTemplate;
            request.presentProgram = prepared->content.presentProgram;
            return request;
        };
        app.renderer.beginScenePreparation(app.renderAssets, makeRequest());
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
        while (app.renderer.scenePreparationStatus().state != ScenePreparationState::Ready &&
            std::chrono::steady_clock::now() < deadline)
        {
            app.renderer.advanceScenePreparation();
            const auto status = app.renderer.scenePreparationStatus();
            if (status.state == ScenePreparationState::Failed)
                throw std::runtime_error(status.error);
            if (status.completed > status.submitted || status.submitted > status.total ||
                app.renderer.sceneReady())
                throw std::runtime_error("preparation exposed incomplete scene resources");
            draw();
        }
        const auto ready = app.renderer.scenePreparationStatus();
        if (ready.state != ScenePreparationState::Ready || ready.completed != ready.total)
            throw std::runtime_error("backend preparation did not finish all uploads and pipelines");
        app.recreateSwapChain(gui);
        draw();
        if (app.renderer.sceneReady())
            throw std::runtime_error("resize activated a prepared scene");
        app.renderer.cancelScenePreparation();
        if (app.renderAssets.materialDescriptorSetLayout() != VK_NULL_HANDLE || app.renderer.sceneReady())
            throw std::runtime_error("ready cancellation left scene resources alive");

        // The backend retains the source even when the caller releases it.
        std::weak_ptr<editor::App::PreparedContent> source = prepared;
        app.renderer.beginScenePreparation(app.renderAssets, makeRequest());
        prepared.reset();
        if (source.expired())
            throw std::runtime_error("backend did not retain the CPU upload source");
        app.renderer.advanceScenePreparation();
        const auto closing = app.renderer.scenePreparationStatus();
        if (closing.submitted <= closing.completed)
            throw std::runtime_error("upload shutdown test did not restart a batch");
        // Exercise the close path while staging and GPU destinations are owned.
        glfwSetWindowShouldClose(app.window.nativeHandle(), GLFW_TRUE);
        app.mainLoop(gui);
        app.discardContentLoading();
        if (!source.expired() ||
            app.renderer.scenePreparationStatus().state != ScenePreparationState::Cancelled)
            throw std::runtime_error("cancellation retained the CPU upload source");
        app.renderer.waitIdle();
        gui.detach();
        app.cleanup();
        if (app.window || app.renderer ||
            app.renderer.scenePreparationStatus().state != ScenePreparationState::Idle ||
            app.contentLoadFuture_.valid())
        {
            throw std::runtime_error("startup shutdown left application resources alive");
        }
        std::clog << "[Startup] First GUI submission: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(firstGui - start).count()
            << " ms; empty resize, CPU cancellation, missing model, preparation restart, ready cancellation and upload shutdown passed\n";
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
