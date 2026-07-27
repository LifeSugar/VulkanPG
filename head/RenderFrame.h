#pragma once

#include "RenderData.h"

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class Mesh;

// Renderer-facing snapshot extracted from the current scene state. It owns no
// GPU resources and makes no assumptions about how a future Scene is stored.
struct RenderView
{
    CameraGpuData cameraData{};
    uint64_t cameraRevision = 0;
};

struct RenderObject
{
    const Mesh* mesh = nullptr;
    ObjectGpuData objectData{};
};

struct RenderFrame
{
    RenderView view;
    std::vector<RenderObject> objects;
};

} // namespace VkRenderer
