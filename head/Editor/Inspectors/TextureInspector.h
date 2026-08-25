#pragma once

#include "Asset/AssetFwd.h"

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;

class TextureInspector final
{
public:
    void draw(
        const AssetManager& assets,
        ApplicationGuiRenderBridge& texturePreviews,
        TextureAssetHandle target) const;
};

} // namespace VkRenderer
