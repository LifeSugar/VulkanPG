#pragma once

#include "EditorFwd.hpp"
#include "EditorSelection.hpp"

#include <optional>

namespace rubia::editor
{

class SceneNodeInspector final
{
public:
    [[nodiscard]] std::optional<InspectorTarget> draw(
        const scene::Scene& scene,
        const asset::AssetManager& assets,
        SceneNodeTarget target) const;
};

} // namespace rubia::editor
