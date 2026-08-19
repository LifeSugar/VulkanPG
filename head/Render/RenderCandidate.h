#pragma once

#include "Asset/AssetFwd.h"
#include "Math/Aabb.h"
#include "Render/CullingSettings.h"
#include "Render/RenderLayer.h"
#include "RenderData.h"

#include <cstdint>

namespace VkRenderer
{

/// View-independent draw candidate extracted from one scene instance.
struct RenderCandidate
{
    /// Stable CPU asset identity; resolved to a GPU mesh during list building.
    MeshAssetHandle mesh;
    /// One independently material-bound draw range within the mesh.
    uint32_t submeshIndex = 0;
    /// Stable CPU asset identity; resolved to a GPU material during list building.
    MaterialAssetHandle material;
    /// Scene-instance layers evaluated against a RenderView culling mask.
    LayerMask layerMask = RenderLayer::World;
    /// Whether this candidate may be rejected by geometric bounds tests.
    BoundsCullingMode boundsCullingMode = BoundsCullingMode::Normal;
    /// Transform data uploaded if this candidate survives list construction.
    ObjectGpuData objectData{};
    /// Conservative mesh bounds transformed into world space.
    Aabb worldBounds;
};

} // namespace VkRenderer
