#pragma once

#include "Render/CullingSystem.h"
#include "Render/RenderList.h"

namespace VkRenderer
{

class AssetManager;
class RenderAssetCache;
struct RenderView;

/// Resolves, classifies, and orders visible candidates for one RenderView.
class RenderListBuilder final
{
public:
    [[nodiscard]] RenderList build(
        const std::vector<RenderCandidate>& candidates,
        const CullingResults& cullingResults,
        const RenderView& view,
        const AssetManager& assets,
        const RenderAssetCache& renderAssets) const;
};

} // namespace VkRenderer
