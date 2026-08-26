#pragma once

#include "Asset/AssetFwd.h"

#include <cstdint>

namespace VkRenderer
{

/// Backend-neutral texture token consumed by an application GUI.
struct ApplicationGuiTexture
{
    std::uintptr_t textureId = 0;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return textureId != 0;
    }
};

/// Renderer information needed by one GUI frame.
struct ApplicationGuiRenderFrame
{
    uint32_t width = 0;
    uint32_t height = 0;
    ApplicationGuiTexture sceneViewport;
};

/// Narrow GUI-facing port that hides renderer and graphics-API resources.
class ApplicationGuiRenderBridge
{
public:
    virtual ~ApplicationGuiRenderBridge() = default;

    /// Ensures the Editor scene image matches the requested drawable size.
    /// Backends without a dedicated scene target may ignore the request.
    virtual void resizeSceneViewport(uint32_t width, uint32_t height)
    {
        static_cast<void>(width);
        static_cast<void>(height);
    }
    [[nodiscard]] virtual ApplicationGuiRenderFrame currentFrame() = 0;
    [[nodiscard]] virtual ApplicationGuiTexture preview(
        TextureAssetHandle texture) = 0;
    /// Drops GUI descriptors that reference the texture's previous GPU view.
    virtual void invalidatePreview(TextureAssetHandle texture) noexcept = 0;
};

} // namespace VkRenderer
