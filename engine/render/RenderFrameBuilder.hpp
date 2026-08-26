#pragma once

#include "render/RenderFrame.hpp"

namespace VkRenderer
{

class AssetManager;
class Scene;

/// Builds renderer input from one scene and view snapshot.
[[nodiscard]] RenderFrame buildRenderFrame(
    const Scene& scene,
    const AssetManager& assets,
    RenderView view);

} // namespace VkRenderer
