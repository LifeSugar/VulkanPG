#pragma once

#include "render/CullingSystem.hpp"
#include "render/RenderList.hpp"

namespace rubia::asset
{
class AssetManager;
}

namespace rubia::render
{
struct RenderView;

/// Resolves, classifies, and orders visible candidates for one RenderView.
class RenderListBuilder final
{
public:
    [[nodiscard]] RenderList build(
        const std::vector<RenderCandidate>& candidates,
        const CullingResults& cullingResults,
        const RenderView& view,
        const asset::AssetManager& assets) const;
};

} // namespace rubia::render
