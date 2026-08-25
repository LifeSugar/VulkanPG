#include "Render/CullingSystem.h"

#include "Math/Frustum.h"
#include "Render/RenderView.h"

#include <limits>
#include <optional>
#include <stdexcept>

namespace VkRenderer
{

CullingResults CullingSystem::cull(
    const std::vector<RenderCandidate>& candidates,
    const RenderView& view) const
{
    if (candidates.size() > std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error(
            "render candidate count exceeds the uint32_t range");
    }

    const bool filterLayers = hasCullingFlag(
        view.cullingFlags,
        CullingFlags::LayerMask);
    const bool testFrustum = hasCullingFlag(
        view.cullingFlags,
        CullingFlags::Frustum);

    std::optional<Frustum> frustum;
    if (testFrustum)
    {
        frustum = Frustum::fromViewProjection(
            view.viewProjectionMatrix);
    }

    CullingResults result{};
    result.inputCount = static_cast<uint32_t>(candidates.size());
    result.visibleCandidateIndices.reserve(candidates.size());

    for (uint32_t index = 0; index < result.inputCount; ++index)
    {
        const RenderCandidate& candidate = candidates[index];
        if (filterLayers &&
            !candidate.layerMask.intersects(view.cullingMask))
        {
            ++result.layerCulledCount;
            continue;
        }

        if (testFrustum &&
            candidate.boundsCullingMode == BoundsCullingMode::Disabled)
        {
            ++result.boundsCullingDisabledCount;
        }
        else if (frustum &&
                 !frustum->intersects(candidate.worldBounds))
        {
            ++result.frustumCulledCount;
            continue;
        }

        result.visibleCandidateIndices.push_back(index);
    }
    return result;
}

} // namespace VkRenderer
