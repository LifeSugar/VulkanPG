#pragma once

#include "RenderFrame.h"

namespace VkRenderer
{

class AssetManager;
class RenderAssetCache;
class Scene;

/// Builds renderer input from one scene and view snapshot.
[[nodiscard]] RenderFrame buildRenderFrame(
    const Scene& scene,
    const AssetManager& assets,
    const RenderAssetCache& renderAssets,
    RenderView view);

} // namespace VkRenderer
