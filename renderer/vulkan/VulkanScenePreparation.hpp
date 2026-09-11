#pragma once

#include "render/SceneResourcePreparation.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/UploadContext.hpp"

#include <exception>
#include <memory>

namespace rubia::rhi::vulkan
{
class RenderAssetCache;
class VulkanRenderer;

/// Backend-internal, render-thread-only preparation of an initially empty
/// scene. Renderer, cache, and device must outlive this session.
class VulkanScenePreparation final
{
public:
    VulkanScenePreparation(const Device& device, VulkanRenderer& renderer, RenderAssetCache& cache,
                           render::SceneResourceRequest request);
    ~VulkanScenePreparation();
    VulkanScenePreparation(const VulkanScenePreparation&) = delete;
    VulkanScenePreparation& operator=(const VulkanScenePreparation&) = delete;

    void begin();
    void advance();
    void activate();
    void cancel() noexcept;
    [[nodiscard]] const render::ScenePreparationStatus& status() const noexcept
    {
        return status_;
    }
    [[nodiscard]] bool ownsResources() const noexcept
    {
        return ownsResources_;
    }

private:
    void releaseUploads() noexcept;
    void discardResources() noexcept;
    void fail(const std::exception& error);

    const Device& device_;
    VulkanRenderer& renderer_;
    RenderAssetCache& cache_;
    render::SceneResourceRequest request_;
    render::ScenePreparationStatus status_;
    bool ownsResources_ = false;
    // UploadContext must be destroyed before its command pool.
    CommandPool pool_;
    std::unique_ptr<UploadContext> uploads_;
};
} // namespace rubia::rhi::vulkan
