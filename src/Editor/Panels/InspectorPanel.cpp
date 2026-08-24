#include "Editor/Panels/InspectorPanel.h"

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"

#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace VkRenderer
{

namespace
{

template <typename... Visitors>
struct Overloaded : Visitors...
{
    using Visitors::operator()...;
};

template <typename... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

[[nodiscard]] const char* displayName(
    const std::string& name,
    const char* fallback) noexcept
{
    return name.empty() ? fallback : name.c_str();
}

void drawProperty(const char* label, const char* value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(120.0f);
    ImGui::TextUnformatted(value);
}

void drawProperty(const char* label, uint32_t value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(120.0f);
    ImGui::Text("%u", value);
}

[[nodiscard]] bool drawReference(
    const char* label,
    const char* value,
    int id)
{
    ImGui::PushID(id);
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(120.0f);
    ImGui::TextUnformatted(value);
    ImGui::SameLine();
    const bool clicked = ImGui::SmallButton("Inspect");
    ImGui::PopID();
    return clicked;
}

[[nodiscard]] const char* textureFormatName(TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::Undefined: return "Undefined";
    case TextureFormat::R8UNorm: return "R8 UNorm";
    case TextureFormat::RG8UNorm: return "RG8 UNorm";
    case TextureFormat::RGBA8UNorm: return "RGBA8 UNorm";
    case TextureFormat::RGBA16Float: return "RGBA16 Float";
    case TextureFormat::RGBA32Float: return "RGBA32 Float";
    }
    return "Unknown";
}

} // namespace

void InspectorPanel::draw(
    const Scene& scene,
    const AssetManager& assets,
    EditorSelection& selection,
    bool* open)
{
    const bool visible = ImGui::Begin("Inspector", open);
    if (!visible)
    {
        ImGui::End();
        return;
    }

    pendingNavigation_.reset();
    if (selection.canNavigateBack() && ImGui::SmallButton("< Back"))
    {
        selection.navigateBack();
    }

    std::visit(
        Overloaded{
            [](std::monostate)
            {
                ImGui::TextDisabled("Select an object to inspect");
            },
            [&](SceneNodeTarget target)
            {
                drawSceneNode(scene, assets, target);
            },
            [&](ModelAssetHandle target)
            {
                drawModelAsset(assets, target);
            },
            [&](ModelNodeTarget target)
            {
                drawModelNode(assets, target);
            },
            [&](MeshAssetHandle target)
            {
                drawMeshAsset(assets, target);
            },
            [&](SubmeshTarget target)
            {
                drawSubmesh(assets, target);
            },
            [&](MaterialAssetHandle target)
            {
                drawMaterialAsset(assets, target);
            },
            [&](MaterialTemplateAssetHandle target)
            {
                drawMaterialTemplate(assets, target);
            },
            [&](TextureAssetHandle target)
            {
                drawTextureAsset(assets, target);
            }
        },
        selection.target());

    if (pendingNavigation_)
    {
        selection.navigateTo(std::move(*pendingNavigation_));
        pendingNavigation_.reset();
    }
    ImGui::End();
}

void InspectorPanel::drawSceneNode(
    const Scene& scene,
    const AssetManager& assets,
    SceneNodeTarget target)
{
    if (target.nodeIndex >= scene.nodes().size())
    {
        ImGui::TextDisabled("SceneNode selection is no longer valid");
        return;
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
            requestNavigation(node.model);
        }
    }
    else
    {
        drawProperty("Model", "None");
    }
}

void InspectorPanel::drawModelAsset(
    const AssetManager& assets,
    ModelAssetHandle target)
{
    if (!assets.contains(target))
    {
        ImGui::TextDisabled("ModelAsset selection is no longer valid");
        return;
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
            requestNavigation(ModelNodeTarget{target, index, std::nullopt});
        }
        ImGui::PopID();
    }
}

void InspectorPanel::drawModelNode(
    const AssetManager& assets,
    ModelNodeTarget target)
{
    if (!assets.contains(target.model) ||
        target.nodeIndex >= assets.model(target.model).nodes().size())
    {
        ImGui::TextDisabled("ModelNode selection is no longer valid");
        return;
    }

    const ModelNode& node =
        assets.model(target.model).nodes()[target.nodeIndex];
    ImGui::SeparatorText("Model Node");
    drawProperty("Name", displayName(node.name, "Unnamed ModelNode"));
    drawProperty("Node Index", target.nodeIndex);
    drawProperty(
        "Parent",
        node.parent == kInvalidModelNodeIndex ? "Model Root" : "ModelNode");

    drawProperty(
        "Mesh References",
        static_cast<uint32_t>(node.meshes.size()));
}

void InspectorPanel::drawMeshAsset(
    const AssetManager& assets,
    MeshAssetHandle target)
{
    if (!assets.contains(target))
    {
        ImGui::TextDisabled("MeshAsset selection is no longer valid");
        return;
    }

    const MeshAsset& mesh = assets.mesh(target);
    ImGui::SeparatorText("Mesh Asset");
    drawProperty("Name", displayName(mesh.name(), "Unnamed Mesh"));
    drawProperty("Vertices", static_cast<uint32_t>(mesh.vertices().size()));
    drawProperty("Indices", static_cast<uint32_t>(mesh.indices().size()));
    drawProperty("Submeshes", static_cast<uint32_t>(mesh.submeshes().size()));
}

void InspectorPanel::drawSubmesh(
    const AssetManager& assets,
    SubmeshTarget target)
{
    if (!assets.contains(target.mesh) ||
        target.submeshIndex >= assets.mesh(target.mesh).submeshes().size())
    {
        ImGui::TextDisabled("Submesh selection is no longer valid");
        return;
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
            requestNavigation(submesh.material);
        }
    }
    else
    {
        drawProperty("Material", "None");
    }
}

void InspectorPanel::drawMaterialAsset(
    const AssetManager& assets,
    MaterialAssetHandle target)
{
    if (!assets.contains(target))
    {
        ImGui::TextDisabled("MaterialAsset selection is no longer valid");
        return;
    }

    const MaterialAsset& material = assets.material(target);
    ImGui::SeparatorText("Material Asset");
    drawProperty("Name", displayName(material.name(), "Unnamed Material"));

    const MaterialTemplateAssetHandle templateHandle =
        material.materialTemplate();
    const MaterialTemplateAsset* materialTemplate = nullptr;
    if (templateHandle && assets.contains(templateHandle))
    {
        materialTemplate = &assets.materialTemplate(templateHandle);
        if (drawReference(
                "Template",
                displayName(materialTemplate->name(), "Unnamed Template"),
                0))
        {
            requestNavigation(templateHandle);
        }
    }

    ImGui::SeparatorText("Textures");
    const std::vector<TextureAssetHandle>& textures = material.textures();
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(textures.size());
         ++index)
    {
        const TextureAssetHandle textureHandle = textures[index];
        if (!assets.contains(textureHandle))
        {
            continue;
        }

        std::string slotName = "Texture " + std::to_string(index);
        if (materialTemplate != nullptr)
        {
            for (const MaterialTextureSlotDesc& slot :
                 materialTemplate->textureSlots())
            {
                if (slot.slot == index)
                {
                    slotName = slot.name;
                    break;
                }
            }
        }

        const TextureAsset& texture = assets.texture(textureHandle);
        if (drawReference(
                slotName.c_str(),
                displayName(texture.name(), "Unnamed Texture"),
                static_cast<int>(index + 1)))
        {
            requestNavigation(textureHandle);
        }
    }
}

void InspectorPanel::drawMaterialTemplate(
    const AssetManager& assets,
    MaterialTemplateAssetHandle target)
{
    if (!assets.contains(target))
    {
        ImGui::TextDisabled("MaterialTemplate selection is no longer valid");
        return;
    }

    const MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(target);
    ImGui::SeparatorText("Material Template");
    drawProperty(
        "Name",
        displayName(materialTemplate.name(), "Unnamed Template"));
    drawProperty(
        "Parameters",
        static_cast<uint32_t>(materialTemplate.parameters().size()));
    drawProperty(
        "Texture Slots",
        static_cast<uint32_t>(materialTemplate.textureSlots().size()));
    drawProperty(
        "Shaders",
        static_cast<uint32_t>(materialTemplate.shaders().size()));
}

void InspectorPanel::drawTextureAsset(
    const AssetManager& assets,
    TextureAssetHandle target)
{
    if (!assets.contains(target))
    {
        ImGui::TextDisabled("TextureAsset selection is no longer valid");
        return;
    }

    const TextureAsset& texture = assets.texture(target);
    ImGui::SeparatorText("Texture Asset");
    drawProperty("Name", displayName(texture.name(), "Unnamed Texture"));
    ImGui::TextDisabled("Size");
    ImGui::SameLine(120.0f);
    ImGui::Text("%u x %u", texture.width(), texture.height());
    drawProperty("Format", textureFormatName(texture.format()));
    drawProperty(
        "Color Space",
        texture.colorSpace() == TextureColorSpace::Srgb ? "sRGB" : "Linear");
    drawProperty(
        "Mip Levels",
        static_cast<uint32_t>(texture.mipLevels().size()));
}

void InspectorPanel::requestNavigation(InspectorTarget target)
{
    pendingNavigation_ = std::move(target);
}

} // namespace VkRenderer
