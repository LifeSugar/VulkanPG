#include "Editor/Panels/InspectorPanel.h"

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

[[nodiscard]] const char* materialValueTypeName(
    MaterialValueType type) noexcept
{
    switch (type)
    {
    case MaterialValueType::Float: return "Float";
    case MaterialValueType::Float2: return "Float2";
    case MaterialValueType::Float3: return "Float3";
    case MaterialValueType::Float4: return "Float4";
    case MaterialValueType::Matrix4: return "Matrix4";
    case MaterialValueType::Int: return "Int";
    case MaterialValueType::UInt: return "UInt";
    case MaterialValueType::Bool: return "Bool";
    }
    return "Unknown";
}

void drawMaterialParameterValue(
    const MaterialParameterDesc& parameter,
    const std::vector<std::byte>& parameterData)
{
    const uint32_t valueSize =
        MaterialTemplateAsset::valueSize(parameter.type);
    if (parameter.byteOffset > parameterData.size() ||
        valueSize > parameterData.size() - parameter.byteOffset)
    {
        ImGui::TextDisabled("Invalid parameter data");
        return;
    }

    const std::byte* source =
        parameterData.data() + parameter.byteOffset;
    switch (parameter.type)
    {
    case MaterialValueType::Float:
    {
        float value = 0.0f;
        std::memcpy(&value, source, sizeof(value));
        ImGui::Text("%.3f", value);
        break;
    }
    case MaterialValueType::Float2:
    case MaterialValueType::Float3:
    case MaterialValueType::Float4:
    {
        const uint32_t componentCount =
            static_cast<uint32_t>(parameter.type) -
            static_cast<uint32_t>(MaterialValueType::Float2) + 2;
        std::array<float, 4> values{};
        std::memcpy(
            values.data(),
            source,
            componentCount * sizeof(float));
        if (componentCount == 2)
        {
            ImGui::Text("(%.3f, %.3f)", values[0], values[1]);
        }
        else if (componentCount == 3)
        {
            ImGui::Text(
                "(%.3f, %.3f, %.3f)",
                values[0],
                values[1],
                values[2]);
        }
        else
        {
            ImGui::Text(
                "(%.3f, %.3f, %.3f, %.3f)",
                values[0],
                values[1],
                values[2],
                values[3]);
        }
        break;
    }
    case MaterialValueType::Matrix4:
    {
        std::array<float, 16> values{};
        std::memcpy(values.data(), source, sizeof(values));
        for (uint32_t row = 0; row < 4; ++row)
        {
            ImGui::Text(
                "[%.3f, %.3f, %.3f, %.3f]",
                values[row],
                values[4 + row],
                values[8 + row],
                values[12 + row]);
        }
        break;
    }
    case MaterialValueType::Int:
    {
        int32_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        ImGui::Text("%d", value);
        break;
    }
    case MaterialValueType::UInt:
    {
        uint32_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        ImGui::Text("%u", value);
        break;
    }
    case MaterialValueType::Bool:
    {
        uint32_t value = 0;
        std::memcpy(&value, source, sizeof(value));
        ImGui::TextUnformatted(value != 0 ? "true" : "false");
        break;
    }
    }
}

[[nodiscard]] ImTextureRef textureReference(
    EditorTexturePreview preview) noexcept
{
    return ImTextureRef(static_cast<ImTextureID>(preview.textureId));
}

[[nodiscard]] ImVec2 fitTexturePreview(
    uint32_t width,
    uint32_t height,
    float maxWidth,
    float maxHeight) noexcept
{
    if (width == 0 || height == 0 ||
        maxWidth <= 0.0f || maxHeight <= 0.0f)
    {
        return {};
    }

    const float aspect =
        static_cast<float>(width) / static_cast<float>(height);
    ImVec2 size{maxWidth, maxWidth / aspect};
    if (size.y > maxHeight)
    {
        size.y = maxHeight;
        size.x = maxHeight * aspect;
    }
    return size;
}

void drawTextureImage(
    EditorTexturePreviewProvider& texturePreviews,
    TextureAssetHandle handle,
    const TextureAsset& texture,
    ImVec2 size)
{
    const EditorTexturePreview preview =
        texturePreviews.preview(handle);
    if (!preview || size.x <= 0.0f || size.y <= 0.0f)
    {
        ImGui::TextDisabled("Preview unavailable");
        return;
    }

    ImGui::ImageWithBg(
        textureReference(preview),
        size,
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f),
        ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
}

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

void InspectorPanel::draw(
    const Scene& scene,
    const AssetManager& assets,
    EditorTexturePreviewProvider& texturePreviews,
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
                drawModelNode(scene, assets, target);
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
                drawMaterialAsset(assets, texturePreviews, target);
            },
            [&](MaterialTemplateAssetHandle target)
            {
                drawMaterialTemplate(assets, target);
            },
            [&](TextureAssetHandle target)
            {
                drawTextureAsset(assets, texturePreviews, target);
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
    const Scene& scene,
    const AssetManager& assets,
    ModelNodeTarget target)
{
    if (!assets.contains(target.model) ||
        target.nodeIndex >= assets.model(target.model).nodes().size())
    {
        ImGui::TextDisabled("ModelNode selection is no longer valid");
        return;
    }

    const ModelAsset& model = assets.model(target.model);
    const ModelNode& node = model.nodes()[target.nodeIndex];
    if (target.sceneNodeIndex &&
        (*target.sceneNodeIndex >= scene.nodes().size() ||
         scene.nodes()[*target.sceneNodeIndex].model != target.model))
    {
        ImGui::TextDisabled("ModelNode scene instance is no longer valid");
        return;
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
    EditorTexturePreviewProvider& texturePreviews,
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
    else
    {
        drawProperty("Template", "Missing");
    }

    if (materialTemplate == nullptr)
    {
        ImGui::SeparatorText("Properties");
        ImGui::TextDisabled(
            "Properties unavailable because the template is missing");
        return;
    }

    if (!ImGui::CollapsingHeader(
            "Properties",
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##MaterialProperties", 3, tableFlags))
    {
        return;
    }

    ImGui::TableSetupColumn(
        "Property",
        ImGuiTableColumnFlags_WidthStretch,
        0.42f);
    ImGui::TableSetupColumn(
        "Value",
        ImGuiTableColumnFlags_WidthStretch,
        0.48f);
    ImGui::TableSetupColumn(
        "##Action",
        ImGuiTableColumnFlags_WidthFixed,
        54.0f);
    ImGui::TableHeadersRow();

    for (const MaterialParameterDesc& parameter :
         materialTemplate->parameters())
    {
        ImGui::PushID(parameter.name.c_str());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(parameter.name.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "%s%s\nByte offset: %u",
                materialValueTypeName(parameter.type),
                parameter.required ? " (required)" : "",
                parameter.byteOffset);
        }
        ImGui::TableSetColumnIndex(1);
        drawMaterialParameterValue(parameter, material.parameterData());
        ImGui::PopID();
    }

    const std::vector<TextureAssetHandle>& textures = material.textures();
    for (const MaterialTextureSlotDesc& slot :
         materialTemplate->textureSlots())
    {
        ImGui::PushID(slot.name.c_str());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(slot.name.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Texture%s\nSlot: %u\n"
                "Image: set %u, binding %u\n"
                "Sampler: set %u, binding %u",
                slot.required ? " (required)" : "",
                slot.slot,
                slot.imageBinding.set,
                slot.imageBinding.binding,
                slot.samplerBinding.set,
                slot.samplerBinding.binding);
        }

        const TextureAssetHandle textureHandle =
            slot.slot < textures.size()
            ? textures[slot.slot]
            : TextureAssetHandle{};
        ImGui::TableSetColumnIndex(1);
        if (textureHandle && assets.contains(textureHandle))
        {
            const TextureAsset& texture = assets.texture(textureHandle);
            constexpr float thumbnailExtent = 36.0f;
            const ImVec2 thumbnailSize = fitTexturePreview(
                texture.width(),
                texture.height(),
                thumbnailExtent,
                thumbnailExtent);
            drawTextureImage(
                texturePreviews,
                textureHandle,
                texture,
                thumbnailSize);
            ImGui::SameLine();
            ImGui::TextUnformatted(
                displayName(texture.name(), "Unnamed Texture"));
            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton("Inspect"))
            {
                requestNavigation(textureHandle);
            }
        }
        else
        {
            ImGui::TextDisabled("None");
        }
        ImGui::PopID();
    }

    ImGui::EndTable();
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
    EditorTexturePreviewProvider& texturePreviews,
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

    ImGui::SeparatorText("Preview");
    const float previewWidth = ImGui::GetContentRegionAvail().x;
    const ImVec2 previewSize = fitTexturePreview(
        texture.width(),
        texture.height(),
        previewWidth,
        320.0f);
    drawTextureImage(
        texturePreviews,
        target,
        texture,
        previewSize);

    ImGui::SeparatorText("Properties");
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
