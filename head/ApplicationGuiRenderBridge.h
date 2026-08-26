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

    [[nodiscard]] virtual ApplicationGuiRenderFrame currentFrame() = 0;
    [[nodiscard]] virtual ApplicationGuiTexture preview(
        TextureAssetHandle texture) = 0;
    /// Drops GUI descriptors that reference the texture's previous GPU view.
    virtual void invalidatePreview(TextureAssetHandle texture) noexcept = 0;
};

} // namespace VkRenderer
