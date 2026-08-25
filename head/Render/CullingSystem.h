#pragma once

#include "Render/RenderCandidate.h"

#include <cstdint>
#include <vector>

namespace VkRenderer
{

struct RenderView;

/// Visible candidate indices and diagnostic counts for one RenderView.
struct CullingResults
{
    std::vector<uint32_t> visibleCandidateIndices;
    uint32_t inputCount = 0;
    uint32_t layerCulledCount = 0;
    uint32_t frustumCulledCount = 0;
    uint32_t boundsCullingDisabledCount = 0;

    [[nodiscard]] uint32_t visibleCount() const noexcept
    {
        return static_cast<uint32_t>(visibleCandidateIndices.size());
    }
};

/// Applies per-view layer filtering and frustum/AABB visibility tests.
class CullingSystem final
{
public:
    [[nodiscard]] CullingResults cull(
        const std::vector<RenderCandidate>& candidates,
        const RenderView& view) const;
};

} // namespace VkRenderer
