#include "vulkan/VulkanScenePreparation.hpp"

#include "asset/AssetManager.hpp"
#include "vulkan/DefaultPipelineFactory.hpp"
#include "vulkan/RenderAssetCache.hpp"
#include "vulkan/VulkanRenderer.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace rubia::rhi::vulkan
{
using render::ScenePreparationState;

VulkanScenePreparation::VulkanScenePreparation(const Device& device, VulkanRenderer& renderer,
                                               RenderAssetCache& cache,
                                               render::SceneResourceRequest request)
    : device_(device), renderer_(renderer), cache_(cache), request_(std::move(request))
{
}

VulkanScenePreparation::~VulkanScenePreparation()
{
    cancel();
}

void VulkanScenePreparation::begin()
{
    try
    {
        if (!request_.assets || request_.models.empty() || request_.maxRenderObjects == 0)
        {
            throw std::invalid_argument(
                "scene preparation requires CPU assets, models and object capacity");
        }
        const auto& assets = *request_.assets;
        if (!assets.contains(request_.materialTemplate) ||
            !assets.isMaterialTemplateCurrent(request_.materialTemplate) ||
            !assets.contains(request_.presentProgram))
        {
            throw std::invalid_argument(
                "scene preparation references an invalid template or program");
        }
        for (auto model : request_.models)
        {
            for (const auto& node : assets.model(model).nodes())
            {
                for (auto mesh : node.meshes)
                {
                    for (const auto& submesh : assets.mesh(mesh).submeshes())
                    {
                        if (assets.material(submesh.material).materialTemplate() !=
                            request_.materialTemplate)
                        {
                            throw std::invalid_argument("scene preparation requires the requested "
                                                        "shared material template");
                        }
                    }
                }
            }
        }

        // Preconditions on an empty renderer/cache are checked by the public
        // renderer entry point. From here on, all partial resources are ours.
        ownsResources_ = true;
        pool_.create(device_, device_.graphicsQueueFamily(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
        uploads_ = std::make_unique<UploadContext>(device_, pool_);
        cache_.beginUpload(device_, assets, request_.models);
        status_.total = cache_.pendingUploadCount();
        status_.state = ScenePreparationState::Uploading;
    }
    catch (const std::exception& error)
    {
        fail(error);
    }
}

void VulkanScenePreparation::advance()
{
    using namespace std::chrono_literals;
    try
    {
        if (status_.state == ScenePreparationState::Uploading)
        {
            if (!uploads_->pollBatch())
            {
                return;
            }
            status_.completed = status_.submitted;
            if (cache_.pendingUploadCount() == 0)
            {
                releaseUploads();
                status_.state = ScenePreparationState::PreparingPipelines;
                return;
            }

            // Soft budgets: one indivisible resource may exceed a limit.
            const auto deadline = std::chrono::steady_clock::now() + 4ms;
            uploads_->beginBatch();
            std::size_t count = 0;
            do
            {
                cache_.uploadNext(device_, *uploads_, *request_.assets);
                ++count;
            } while (cache_.pendingUploadCount() != 0 && count < 16 &&
                     uploads_->stagedByteCount() < 16 * 1024 * 1024 &&
                     std::chrono::steady_clock::now() < deadline);
            uploads_->submitBatch();
            status_.submitted = status_.total - cache_.pendingUploadCount();
        }
        else if (status_.state == ScenePreparationState::PreparingPipelines)
        {
            const auto& assets = *request_.assets;
            renderer_.createSceneResources(
                makeDefaultScenePipeline(
                    assets, assets.materialTemplate(request_.materialTemplate).program(),
                    cache_.materialDescriptorSetLayout()),
                makeDefaultPresentPipeline(assets, request_.presentProgram),
                request_.maxRenderObjects);
            status_.state = ScenePreparationState::Ready;
        }
    }
    catch (const std::exception& error)
    {
        fail(error);
    }
}

void VulkanScenePreparation::activate()
{
    if (status_.state != ScenePreparationState::Ready)
    {
        throw std::logic_error("scene preparation must be ready before activation");
    }
    // Completed GPU objects stay in renderer/cache. The source owner may now
    // move its CPU registries into the live application without backend reads.
    ownsResources_ = false;
    request_ = {};
    status_.state = ScenePreparationState::Activated;
}

void VulkanScenePreparation::releaseUploads() noexcept
{
    // Drains an in-flight batch, or discards unsubmitted commands, before any
    // referenced destination or source is released. Normal advance only polls.
    uploads_.reset();
    pool_.reset();
}

void VulkanScenePreparation::discardResources() noexcept
{
    releaseUploads();
    if (ownsResources_)
    {
        ownsResources_ = false;
        renderer_.releaseSceneResources();
        cache_.reset();
    }
    request_ = {};
}

void VulkanScenePreparation::cancel() noexcept
{
    if (status_.state == ScenePreparationState::Activated ||
        status_.state == ScenePreparationState::Cancelled ||
        status_.state == ScenePreparationState::Failed)
    {
        return;
    }
    discardResources();
    status_.state = ScenePreparationState::Cancelled;
}

void VulkanScenePreparation::fail(const std::exception& error)
{
    status_.error = error.what();
    discardResources();
    status_.state = ScenePreparationState::Failed;
}
} // namespace rubia::rhi::vulkan
