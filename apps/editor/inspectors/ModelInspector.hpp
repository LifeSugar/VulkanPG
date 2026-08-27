#pragma once

#include "EditorFwd.hpp"
#include "EditorSelection.hpp"
#include "panels/TransformPanel.hpp"

#include <optional>

namespace rubia::editor
{

class ModelInspector final
{
public:
    [[nodiscard]] std::optional<InspectorTarget> drawModelAsset(
        const asset::AssetManager& assets,
        asset::ModelAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawModelNode(
        const scene::Scene& scene,
        const asset::AssetManager& assets,
        ModelNodeTarget target);
    [[nodiscard]] std::optional<InspectorTarget> drawMeshAsset(
        const asset::AssetManager& assets,
        asset::MeshAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawSubmesh(
        const asset::AssetManager& assets,
        SubmeshTarget target) const;

private:
    TransformPanel transformPanel_;
};

} // namespace rubia::editor
