#pragma once

#include "Asset/AssetFwd.h"
#include "Render/MaterialKey.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderQueue.h"

#include <cstdint>

namespace VkRenderer
{

/// One view-specific, backend-neutral draw produced from a visible candidate.
struct RenderItem
{
    /// Stable asset identities resolved by the active render backend.
    MeshAssetHandle mesh;
    MaterialAssetHandle material;

    /// Stable material binding identity and material-controlled PSO variant.
    MaterialKey materialKey;
    PipelineVariantKey pipelineKey;

    /// Source identity used as a deterministic sorting/debugging tie breaker.
    uint32_t candidateIndex = 0;
    /// Draw range selected from the resolved mesh.
    uint32_t submeshIndex = 0;
    /// Index into RenderList::objectData, stable when items are reordered.
    uint32_t objectIndex = 0;
    /// Coarse material ordering group evaluated before finer sort criteria.
    RenderQueue queue = RenderQueue::Opaque;
    /// Positive camera-space depth used by front/back depth ordering.
    float viewDepth = 0.0f;
};

} // namespace VkRenderer
