#include "Editor/Panels/SceneHierarchyPanel.h"

#include "Asset/AssetManager.h"
#include "Asset/ModelAsset.h"
#include "Scene/Scene.h"

#include <imgui.h>

#include <functional>
#include <string>
#include <vector>

namespace VkRenderer
{

namespace
{

struct ModelHierarchy final
{
    std::vector<uint32_t> roots;
    std::vector<std::vector<uint32_t>> children;
};

[[nodiscard]] ModelHierarchy buildModelHierarchy(const ModelAsset& model)
{
    const std::vector<ModelNode>& nodes = model.nodes();
    ModelHierarchy hierarchy{};
    hierarchy.children.resize(nodes.size());
    hierarchy.roots.reserve(nodes.size());

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        const uint32_t parent = nodes[index].parent;
        if (parent == kInvalidModelNodeIndex)
        {
            hierarchy.roots.push_back(index);
        }
        else if (parent < nodes.size())
        {
            hierarchy.children[parent].push_back(index);
        }
    }
    return hierarchy;
}

void drawModelHierarchy(
    const ModelAsset& model,
    uint32_t sceneNodeIndex)
{
    const std::vector<ModelNode>& nodes = model.nodes();
    if (nodes.empty())
    {
        return;
    }

    const ModelHierarchy hierarchy = buildModelHierarchy(model);
    ImGui::PushID("ModelAssetHierarchy");
    ImGui::PushID(static_cast<int>(sceneNodeIndex));

    const char* modelName = model.name().empty()
        ? "Unnamed Model"
        : model.name().c_str();
    const ImGuiTreeNodeFlags modelFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool modelOpen = ImGui::TreeNodeEx(
        "##ModelAsset",
        modelFlags,
        "Model: %s",
        modelName);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Read-only ModelAsset hierarchy");
    }

    if (modelOpen)
    {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

        std::function<void(uint32_t)> drawModelNode =
            [&](uint32_t nodeIndex)
            {
                const ModelNode& node = nodes[nodeIndex];
                const bool hasChildren =
                    !hierarchy.children[nodeIndex].empty();

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hasChildren)
                {
                    flags |= ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                ImGui::PushID(static_cast<int>(nodeIndex));
                const char* label = node.name.empty()
                    ? "Unnamed ModelNode"
                    : node.name.c_str();
                const bool nodeOpen = ImGui::TreeNodeEx(label, flags);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("ModelNode (read-only)");
                }
                if (nodeOpen && hasChildren)
                {
                    for (uint32_t childIndex :
                         hierarchy.children[nodeIndex])
                    {
                        drawModelNode(childIndex);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            };

        for (uint32_t rootIndex : hierarchy.roots)
        {
            drawModelNode(rootIndex);
        }
        ImGui::PopStyleColor();
        ImGui::TreePop();
    }

    ImGui::PopID();
    ImGui::PopID();
}

} // namespace

void SceneHierarchyPanel::draw(
    const Scene& scene,
    const AssetManager& assets,
    bool* open)
{
    const bool visible = ImGui::Begin("Scene Hierarchy", open);
    if (!visible)
    {
        ImGui::End();
        return;
    }

    const std::vector<SceneNode>& nodes = scene.nodes();
    if (selectedNodeIndex_ >= nodes.size())
    {
        clearSelection();
    }

    std::vector<uint32_t> roots;
    std::vector<std::vector<uint32_t>> children(nodes.size());
    roots.reserve(nodes.size());
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        const uint32_t parent = nodes[index].parent;
        if (parent == kInvalidSceneNodeIndex)
        {
            roots.push_back(index);
        }
        else
        {
            children[parent].push_back(index);
        }
    }

    const std::string sceneLabel = scene.name().empty()
        ? "Untitled Scene"
        : scene.name();
    const ImGuiTreeNodeFlags sceneFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool sceneOpen = ImGui::TreeNodeEx(
        "##SceneRoot",
        sceneFlags,
        "%s",
        sceneLabel.c_str());

    if (sceneOpen)
    {
        std::function<void(uint32_t)> drawNode =
            [&](uint32_t nodeIndex)
            {
                const SceneNode& node = nodes[nodeIndex];
                const ModelAsset* model = nullptr;
                if (node.model && assets.contains(node.model))
                {
                    model = &assets.model(node.model);
                }
                const bool hasModelHierarchy =
                    model != nullptr && !model->nodes().empty();
                const bool hasChildren =
                    !children[nodeIndex].empty() || hasModelHierarchy;

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedNodeIndex_ == nodeIndex)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                if (!hasChildren)
                {
                    flags |= ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                else if (hasModelHierarchy)
                {
                    // Make the imported hierarchy visible on first launch.
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }

                ImGui::PushID(static_cast<int>(nodeIndex));
                const char* label = node.name.empty()
                    ? "Unnamed SceneNode"
                    : node.name.c_str();
                const bool nodeOpen = ImGui::TreeNodeEx(label, flags);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    selectedNodeIndex_ = nodeIndex;
                }

                if (nodeOpen && hasChildren)
                {
                    for (uint32_t childIndex : children[nodeIndex])
                    {
                        drawNode(childIndex);
                    }
                    if (model != nullptr)
                    {
                        drawModelHierarchy(*model, nodeIndex);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            };

        for (uint32_t rootIndex : roots)
        {
            drawNode(rootIndex);
        }
        ImGui::TreePop();
    }

    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        clearSelection();
    }
    ImGui::End();
}

std::optional<uint32_t> SceneHierarchyPanel::selectedNodeIndex() const
    noexcept
{
    if (selectedNodeIndex_ == std::numeric_limits<uint32_t>::max())
    {
        return std::nullopt;
    }
    return selectedNodeIndex_;
}

void SceneHierarchyPanel::clearSelection() noexcept
{
    selectedNodeIndex_ = std::numeric_limits<uint32_t>::max();
}

} // namespace VkRenderer
