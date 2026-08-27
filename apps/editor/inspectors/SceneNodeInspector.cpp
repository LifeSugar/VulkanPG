#include "inspectors/SceneNodeInspector.hpp"

#include "asset/AssetManager.hpp"
#include "inspectors/InspectorWidgets.hpp"
#include "scene/Scene.hpp"

#include <imgui.h>

namespace rubia::editor
{

std::optional<InspectorTarget> SceneNodeInspector::draw(
    const scene::Scene& scene,
    const asset::AssetManager& assets,
    SceneNodeTarget target) const
{
    using namespace widgets;

    if (target.nodeIndex >= scene.nodes().size())
    {
        ImGui::TextDisabled("SceneNode selection is no longer valid");
        return std::nullopt;
    }

    const scene::SceneNode& node = scene.nodes()[target.nodeIndex];
    ImGui::SeparatorText("Scene Node");
    drawProperty("Name", displayName(node.name, "Unnamed SceneNode"));
    drawProperty("Node Index", target.nodeIndex);
    drawProperty(
        "Parent",
        node.parent == scene::kInvalidSceneNodeIndex ? "Scene Root" : "SceneNode");

    ImGui::SeparatorText("Components");
    if (node.model && assets.contains(node.model))
    {
        const asset::ModelAsset& model = assets.model(node.model);
        if (drawReference(
                "Model",
                displayName(model.name(), "Unnamed Model"),
                0))
        {
            return InspectorTarget{node.model};
        }
    }
    else
    {
        drawProperty("Model", "None");
    }

    return std::nullopt;
}

} // namespace rubia::editor
