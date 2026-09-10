#include "App.hpp"

#include "vulkan/DefaultPipelineFactory.hpp"
#include "vulkan/UploadContext.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace rubia::editor
{

void App::startContentLoading()
{
    if (contentLoadStatus_.state != ContentLoadState::Idle &&
        contentLoadStatus_.state != ContentLoadState::Failed)
    {
        return;
    }
    discardContentLoading();
    contentLoadStatus_ = {ContentLoadState::Preparing,
        "Reading model and decoding textures..."};
    std::clog << "[Content] " << contentLoadStatus_.message
        << " " << contentLoadConfig_.modelPath.string() << '\n';
    try
    {
        contentLoadCancelled_ = std::make_shared<std::atomic<bool>>(false);
        contentLoadFuture_ = std::async(std::launch::async,
            [config = contentLoadConfig_, cancelled = contentLoadCancelled_]()
            {
                const auto checkpoint = [&cancelled]
                {
                    if (cancelled->load(std::memory_order_relaxed))
                    {
                        throw std::runtime_error("Content loading cancelled");
                    }
                };
                auto prepared = std::make_unique<PreparedContent>();
                prepared->content = DemoContentLoader::load(
                    prepared->assets, prepared->scene, config,
                    &prepared->textureImports, checkpoint);
                checkpoint();
                return prepared;
            });
    }
    catch (const std::exception& error)
    {
        contentLoadStatus_ = {ContentLoadState::Failed, error.what()};
        std::cerr << "[Content] " << error.what() << '\n';
    }
}

void App::updateContentLoading()
{
    using namespace std::chrono_literals;
    try
    {
        if (contentLoadStatus_.state == ContentLoadState::Preparing)
        {
            if (contentLoadFuture_.wait_for(0ms) != std::future_status::ready)
            {
                return;
            }
            preparedContent_ = contentLoadFuture_.get();
            const auto& device = vulkanContext.device();
            contentUploadPool_ = std::make_unique<rhi::vulkan::CommandPool>(
                device, device.graphicsQueueFamily(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
            contentUploads_ = std::make_unique<rhi::vulkan::UploadContext>(
                device, *contentUploadPool_);
            renderAssets.beginUpload(device, preparedContent_->assets,
                {preparedContent_->content.model});
            contentLoadStatus_ = {ContentLoadState::Uploading,
                "Uploading scene resources...", 0, renderAssets.pendingUploadCount()};
            std::clog << "[Content] " << contentLoadStatus_.message
                << " 0/" << contentLoadStatus_.total << '\n';
            return;
        }
        if (contentLoadStatus_.state == ContentLoadState::Uploading)
        {
            // Staging buffers and destinations stay alive until the submitted
            // batch completes. No queue-idle wait is used in the frame loop.
            if (!contentUploads_->pollBatch())
            {
                return;
            }
            const std::size_t previousCompleted = contentLoadStatus_.completed;
            contentLoadStatus_.completed = contentLoadStatus_.total -
                renderAssets.pendingUploadCount();
            // Log at most once per 10% milestone, after GPU completion, so
            // progress remains visible in Console without per-frame spam.
            if (contentLoadStatus_.total != 0 &&
                contentLoadStatus_.completed * 10 / contentLoadStatus_.total >
                    previousCompleted * 10 / contentLoadStatus_.total)
            {
                std::clog << "[Content] Uploaded " << contentLoadStatus_.completed
                    << '/' << contentLoadStatus_.total << " resources ("
                    << contentLoadStatus_.completed * 100 / contentLoadStatus_.total
                    << "%)\n";
            }
            if (renderAssets.pendingUploadCount() == 0)
            {
                contentLoadStatus_.state = ContentLoadState::Finalizing;
                contentLoadStatus_.message = "Preparing scene rendering...";
                std::clog << "[Content] " << contentLoadStatus_.message << '\n';
                return;
            }
            const auto deadline = std::chrono::steady_clock::now() + 4ms;
            contentUploads_->beginBatch();
            std::size_t count = 0;
            do
            {
                renderAssets.uploadNext(vulkanContext.device(), *contentUploads_,
                    preparedContent_->assets);
                ++count;
            } while (renderAssets.pendingUploadCount() != 0 && count < 16 &&
                contentUploads_->stagedByteCount() < 16 * 1024 * 1024 &&
                std::chrono::steady_clock::now() < deadline);
            contentUploads_->submitBatch();
            return;
        }
        if (contentLoadStatus_.state == ContentLoadState::Finalizing)
        {
            const auto& prepared = *preparedContent_;
            renderer.createSceneResources(
                rhi::vulkan::makeDefaultScenePipeline(
                    prepared.assets,
                    prepared.assets.materialTemplate(
                        prepared.content.materialTemplate).program(),
                    renderAssets.materialDescriptorSetLayout()),
                rhi::vulkan::makeDefaultPresentPipeline(
                    prepared.assets,
                    prepared.content.presentProgram));

            // No partially imported assets are exposed to inspectors. Handles
            // remain valid because the complete registries move together.
            static_assert(std::is_nothrow_move_assignable_v<asset::AssetManager>);
            static_assert(std::is_nothrow_move_assignable_v<scene::Scene>);
            static_assert(std::is_nothrow_move_assignable_v<importer::texture::TextureImportRegistry>);
            assetManager = std::move(preparedContent_->assets);
            scene = std::move(preparedContent_->scene);
            textureImports = std::move(preparedContent_->textureImports);
            demoContent = preparedContent_->content;
            discardContentLoading();
            contentLoadStatus_ = {ContentLoadState::Ready, "Scene ready"};
            std::clog << "[Content] Scene ready\n";
        }
    }
    catch (const std::exception& error)
    {
        // Drain any submitted upload before destroying its destination objects.
        discardContentLoading();
        renderer.resetSceneResources();
        renderAssets.reset();
        contentLoadStatus_ = {ContentLoadState::Failed, error.what()};
        std::cerr << "[Content] " << error.what() << '\n';
    }
}

void App::discardContentLoading() noexcept
{
    if (contentLoadCancelled_)
    {
        contentLoadCancelled_->store(true, std::memory_order_relaxed);
    }
    // Join the worker before GUI log capture and application resources die.
    // Parsing/decoding a single source is allowed to finish before cancellation.
    if (contentLoadFuture_.valid())
    {
        try { static_cast<void>(contentLoadFuture_.get()); }
        catch (...) {}
    }
    contentUploads_.reset();
    contentUploadPool_.reset();
    preparedContent_.reset();
    contentLoadCancelled_.reset();
}

} // namespace rubia::editor
