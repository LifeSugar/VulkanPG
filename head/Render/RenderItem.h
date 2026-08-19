#pragma once

#include "Asset/AssetFwd.h"
#include "Render/MaterialKey.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderQueue.h"

#include <cstdint>

namespace VkRenderer
{

class GpuMaterial;
class Mesh;

/// One view-specific, backend-ready draw produced from a visible candidate.
struct RenderItem
{
    /// Non-owning GPU resources valid for the lifetime of the render asset cache.
    const Mesh* mesh = nullptr;
    const GpuMaterial* material = nullptr;

    /// Stable asset identities retained for sorting, diagnostics, and cache keys.
    MeshAssetHandle meshHandle;
    MaterialAssetHandle materialHandle;

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
