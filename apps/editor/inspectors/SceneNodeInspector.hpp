#pragma once

#include "EditorSelection.hpp"

#include <optional>

namespace VkRenderer
{

class AssetManager;
class Scene;

class SceneNodeInspector final
{
public:
    [[nodiscard]] std::optional<InspectorTarget> draw(
        const Scene& scene,
        const AssetManager& assets,
        SceneNodeTarget target) const;
};

} // namespace VkRenderer
