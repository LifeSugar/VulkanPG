#pragma once

#include "render/CullingSystem.hpp"
#include "render/RenderList.hpp"

namespace VkRenderer
{

class AssetManager;
struct RenderView;

/// Resolves, classifies, and orders visible candidates for one RenderView.
class RenderListBuilder final
{
public:
    [[nodiscard]] RenderList build(
        const std::vector<RenderCandidate>& candidates,
        const CullingResults& cullingResults,
        const RenderView& view,
        const AssetManager& assets) const;
};

} // namespace VkRenderer
