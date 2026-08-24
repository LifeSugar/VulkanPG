#pragma once

#include "Asset/AssetFwd.h"

#include <cstdint>

namespace VkRenderer
{

/// Backend-neutral texture token consumed by Editor ImGui panels.
struct EditorTexturePreview
{
    std::uintptr_t textureId = 0;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return textureId != 0;
    }
};

/// Keeps renderer-specific texture registration outside individual panels.
class EditorTexturePreviewProvider
{
public:
    virtual ~EditorTexturePreviewProvider() = default;

    [[nodiscard]] virtual EditorTexturePreview preview(
        TextureAssetHandle texture) = 0;
};

} // namespace VkRenderer
