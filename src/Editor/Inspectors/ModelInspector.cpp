#include "Editor/Inspectors/ModelInspector.h"

#include "Asset/AssetManager.h"
#include "Editor/Inspectors/InspectorWidgets.h"
#include "Scene/Scene.h"

#include <imgui.h>

#include <vector>

namespace VkRenderer
{

namespace
{

[[nodiscard]] glm::mat4 sceneNodeWorldTransform(
    const Scene& scene,
    uint32_t nodeIndex)
{
    const std::vector<SceneNode>& nodes = scene.nodes();
    std::vector<uint32_t> ancestors;
    uint32_t current = nodeIndex;
    while (current != kInvalidSceneNodeIndex)
    {
        ancestors.push_back(current);
        current = nodes[current].parent;
    }

    glm::mat4 world(1.0f);
    for (auto iterator = ancestors.rbegin();
         iterator != ancestors.rend();
         ++iterator)
    {
        world *= nodes[*iterator].localTransform;
    }
    return world;
}

} // namespace

std::optional<InspectorTarget> ModelInspector::drawModelAsset(
    const AssetManager& assets,
    ModelAssetHandle target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled("ModelAsset selection is no longer valid");
        return std::nullopt;
    }

    const ModelAsset& model = assets.model(target);
    ImGui::SeparatorText("Model Asset");
    drawProperty("Name", displayName(model.name(), "Unnamed Model"));
    drawProperty("Nodes", static_cast<uint32_t>(model.nodes().size()));

    ImGui::SeparatorText("Model Nodes");
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(model.nodes().size());
         ++index)
    {
        ImGui::PushID(static_cast<int>(index));
        const ModelNode& node = model.nodes()[index];
        if (ImGui::Selectable(displayName(node.name, "Unnamed ModelNode")))
        {
            ImGui::PopID();
            return InspectorTarget{
                ModelNodeTarget{target, index, std::nullopt}};
        }
        ImGui::PopID();
    }

    return std::nullopt;
}

std::optional<InspectorTarget> ModelInspector::drawModelNode(
    const Scene& scene,
    const AssetManager& assets,
    ModelNodeTarget target)
{
    using namespace InspectorWidgets;

    if (!assets.contains(target.model) ||
        target.nodeIndex >= assets.model(target.model).nodes().size())
    {
        ImGui::TextDisabled("ModelNode selection is no longer valid");
        return std::nullopt;
    }

    const ModelAsset& model = assets.model(target.model);
    const ModelNode& node = model.nodes()[target.nodeIndex];
    if (target.sceneNodeIndex &&
        (*target.sceneNodeIndex >= scene.nodes().size() ||
         scene.nodes()[*target.sceneNodeIndex].model != target.model))
    {
        ImGui::TextDisabled("ModelNode scene instance is no longer valid");
        return std::nullopt;
    }

    ImGui::SeparatorText("Model Node");
    drawProperty("Name", displayName(node.name, "Unnamed ModelNode"));
    drawProperty("Node Index", target.nodeIndex);
    drawProperty(
        "Parent",
        node.parent == kInvalidModelNodeIndex ? "Model Root" : "ModelNode");
    drawProperty(
        "Mesh References",
        static_cast<uint32_t>(node.meshes.size()));

    glm::mat4 inspectedTransform = node.localTransform;
    TransformPanel::Space transformSpace =
        TransformPanel::Space::LocalToParent;
    if (node.parent == kInvalidModelNodeIndex)
    {
        transformSpace = TransformPanel::Space::World;
        if (target.sceneNodeIndex)
        {
            inspectedTransform = sceneNodeWorldTransform(
                scene,
                *target.sceneNodeIndex) * node.localTransform;
        }
    }
    transformPanel_.draw(inspectedTransform, transformSpace);
    return std::nullopt;
}

std::optional<InspectorTarget> ModelInspector::drawMeshAsset(
    const AssetManager& assets,
    MeshAssetHandle target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled("MeshAsset selection is no longer valid");
        return std::nullopt;
    }

    const MeshAsset& mesh = assets.mesh(target);
    ImGui::SeparatorText("Mesh Asset");
    drawProperty("Name", displayName(mesh.name(), "Unnamed Mesh"));
    drawProperty("Vertices", static_cast<uint32_t>(mesh.vertices().size()));
    drawProperty("Indices", static_cast<uint32_t>(mesh.indices().size()));
    drawProperty("Submeshes", static_cast<uint32_t>(mesh.submeshes().size()));
    return std::nullopt;
}

std::optional<InspectorTarget> ModelInspector::drawSubmesh(
    const AssetManager& assets,
    SubmeshTarget target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target.mesh) ||
        target.submeshIndex >= assets.mesh(target.mesh).submeshes().size())
    {
        ImGui::TextDisabled("Submesh selection is no longer valid");
        return std::nullopt;
    }

    const SubmeshData& submesh =
        assets.mesh(target.mesh).submeshes()[target.submeshIndex];
    ImGui::SeparatorText("Submesh");
    drawProperty("Submesh Index", target.submeshIndex);
    drawProperty("Vertices", submesh.vertexCount);
    drawProperty("Indices", submesh.indexCount);

    if (submesh.material && assets.contains(submesh.material))
    {
        const MaterialAsset& material = assets.material(submesh.material);
        if (drawReference(
                "Material",
                displayName(material.name(), "Unnamed Material"),
                0))
        {
            return InspectorTarget{submesh.material};
        }
    }
    else
    {
        drawProperty("Material", "None");
    }

    return std::nullopt;
}

} // namespace VkRenderer
