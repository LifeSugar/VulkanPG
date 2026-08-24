#pragma once

#include "Editor/EditorSelection.h"
#include "Editor/Panels/TransformPanel.h"

#include <optional>

namespace VkRenderer
{

class AssetManager;
class Scene;

class ModelInspector final
{
public:
    [[nodiscard]] std::optional<InspectorTarget> drawModelAsset(
        const AssetManager& assets,
        ModelAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawModelNode(
        const Scene& scene,
        const AssetManager& assets,
        ModelNodeTarget target);
    [[nodiscard]] std::optional<InspectorTarget> drawMeshAsset(
        const AssetManager& assets,
        MeshAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawSubmesh(
        const AssetManager& assets,
        SubmeshTarget target) const;

private:
    TransformPanel transformPanel_;
};

} // namespace VkRenderer
