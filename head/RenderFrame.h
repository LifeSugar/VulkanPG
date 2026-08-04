#pragma once

#include "RenderData.h"

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class Mesh;
class GpuMaterial;

// Renderer-facing snapshot extracted from the current scene state. It owns no
// GPU resources and makes no assumptions about how a future Scene is stored.
struct RenderView
{
    /// Camera data consumed by the current frame's shaders.
    CameraGpuData cameraData{};
    /// Monotonic revision used to skip unchanged camera uploads.
    uint64_t cameraRevision = 0;
};

/// One mesh instance and its per-object shader data.
struct RenderObject
{
    /// Non-owning mesh reference used to issue draw commands.
    const Mesh* mesh = nullptr;
    /// One independently material-bound draw range within the mesh.
    uint32_t submeshIndex = 0;
    /// Non-owning material descriptors bound for this draw.
    const GpuMaterial* material = nullptr;
    /// Transform data uploaded for this object.
    ObjectGpuData objectData{};
};

/// Complete renderer input for one frame.
struct RenderFrame
{
    /// Camera snapshot shared by all objects in the frame.
    RenderView view;
    /// Ordered list of objects to draw.
    std::vector<RenderObject> objects;
};

} // namespace VkRenderer
