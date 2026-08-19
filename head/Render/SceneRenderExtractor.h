#pragma once

#include "Render/RenderCandidate.h"

#include <vector>

namespace VkRenderer
{

class AssetManager;
class Scene;

/// Expands Scene instances and ModelAsset hierarchies into flat submesh draws.
class SceneRenderExtractor final
{
public:
    [[nodiscard]] std::vector<RenderCandidate> extract(
        const Scene& scene,
        const AssetManager& assets) const;
};

} // namespace VkRenderer
