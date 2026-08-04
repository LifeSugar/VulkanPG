#pragma once

#include "Asset/AssetManager.h"
#include "RenderAssetCache.h"
#include "RenderFrame.h"
#include "Scene/Scene.h"

namespace VkRenderer
{

/// Expands Scene instances and ModelAsset hierarchies into flat submesh draws.
class SceneRenderExtractor final
{
public:
    [[nodiscard]] RenderFrame extract(
        const Scene& scene,
        const AssetManager& assets,
        const RenderAssetCache& renderAssets,
        RenderView view) const;
};

} // namespace VkRenderer
