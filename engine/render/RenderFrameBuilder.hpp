#pragma once

#include "render/RenderFrame.hpp"

namespace rubia::asset
{
class AssetManager;
}

namespace rubia::scene
{
class Scene;
}

namespace rubia::render
{

/// Builds renderer input from one scene and view snapshot.
[[nodiscard]] RenderFrame buildRenderFrame(
    const scene::Scene& scene,
    const asset::AssetManager& assets,
    RenderView view);

} // namespace rubia::render
