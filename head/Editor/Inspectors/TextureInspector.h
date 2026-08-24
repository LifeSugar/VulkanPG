#pragma once

#include "Asset/AssetFwd.h"

namespace VkRenderer
{

class AssetManager;
class EditorTexturePreviewProvider;

class TextureInspector final
{
public:
    void draw(
        const AssetManager& assets,
        EditorTexturePreviewProvider& texturePreviews,
        TextureAssetHandle target) const;
};

} // namespace VkRenderer
