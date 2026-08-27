#pragma once

#include "render/RenderCandidate.hpp"

#include <vector>

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

/// Expands Scene instances and ModelAsset hierarchies into flat submesh draws.
class SceneRenderExtractor final
{
public:
    [[nodiscard]] std::vector<RenderCandidate> extract(
        const scene::Scene& scene,
        const asset::AssetManager& assets) const;
};

} // namespace rubia::render
