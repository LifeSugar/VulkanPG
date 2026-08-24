#include "Editor/Inspectors/SceneNodeInspector.h"

#include "Asset/AssetManager.h"
#include "Editor/Inspectors/InspectorWidgets.h"
#include "Scene/Scene.h"

#include <imgui.h>

namespace VkRenderer
{

std::optional<InspectorTarget> SceneNodeInspector::draw(
    const Scene& scene,
    const AssetManager& assets,
    SceneNodeTarget target) const
{
    using namespace InspectorWidgets;

    if (target.nodeIndex >= scene.nodes().size())
    {
        ImGui::TextDisabled("SceneNode selection is no longer valid");
        return std::nullopt;
    }

    const SceneNode& node = scene.nodes()[target.nodeIndex];
    ImGui::SeparatorText("Scene Node");
    drawProperty("Name", displayName(node.name, "Unnamed SceneNode"));
    drawProperty("Node Index", target.nodeIndex);
    drawProperty(
        "Parent",
        node.parent == kInvalidSceneNodeIndex ? "Scene Root" : "SceneNode");

    ImGui::SeparatorText("Components");
    if (node.model && assets.contains(node.model))
    {
        const ModelAsset& model = assets.model(node.model);
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

} // namespace VkRenderer
