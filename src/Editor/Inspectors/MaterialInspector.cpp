#include "Editor/Inspectors/MaterialInspector.h"

#include "Asset/AssetManager.h"
#include "ApplicationGuiRenderBridge.h"
#include "Editor/Inspectors/InspectorWidgets.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderQueue.h"

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace VkRenderer
{

namespace
{

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

[[nodiscard]] const char* shaderStageName(ShaderStage stage) noexcept
{
    switch (stage)
    {
    case ShaderStage::Vertex: return "Vertex";
    case ShaderStage::Fragment: return "Fragment";
    case ShaderStage::Compute: return "Compute";
    }
    return "Unknown";
}

[[nodiscard]] const char* renderQueueName(RenderQueue queue) noexcept
{
    switch (queue)
    {
    case RenderQueue::Opaque: return "Opaque";
    case RenderQueue::AlphaClip: return "Alpha Clip";
    case RenderQueue::Transparent: return "Transparent";
    }
    return "Unknown";
}

[[nodiscard]] const char* depthCompareName(DepthCompare compare) noexcept
{
    switch (compare)
    {
    case DepthCompare::Never: return "Never";
    case DepthCompare::Less: return "Less";
    case DepthCompare::Equal: return "Equal";
    case DepthCompare::LessEqual: return "Less Equal";
    case DepthCompare::Greater: return "Greater";
    case DepthCompare::NotEqual: return "Not Equal";
    case DepthCompare::GreaterEqual: return "Greater Equal";
    case DepthCompare::Always: return "Always";
    }
    return "Unknown";
}

[[nodiscard]] const char* blendModeName(PipelineBlendMode mode) noexcept
{
    switch (mode)
    {
    case PipelineBlendMode::Disabled: return "Disabled";
    case PipelineBlendMode::Alpha: return "Alpha";
    }
    return "Unknown";
}

[[nodiscard]] const char* cullModeName(PipelineCullMode mode) noexcept
{
    switch (mode)
    {
    case PipelineCullMode::None: return "None";
    case PipelineCullMode::Front: return "Front";
    case PipelineCullMode::Back: return "Back";
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

    const std::byte* source = parameterData.data() + parameter.byteOffset;
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

} // namespace

std::optional<InspectorTarget> MaterialInspector::drawMaterialAsset(
    const AssetManager& assets,
    ApplicationGuiRenderBridge& texturePreviews,
    MaterialAssetHandle target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled("MaterialAsset selection is no longer valid");
        return std::nullopt;
    }

    std::optional<InspectorTarget> navigation;
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
            navigation = InspectorTarget{templateHandle};
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
        return navigation;
    }

    if (ImGui::CollapsingHeader("Shaders", ImGuiTreeNodeFlags_DefaultOpen))
    {
        drawProperty("Source", "Material Template");
        constexpr ImGuiTableFlags shaderTableFlags =
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("##MaterialShaders", 3, shaderTableFlags))
        {
            ImGui::TableSetupColumn(
                "Stage",
                ImGuiTableColumnFlags_WidthFixed,
                72.0f);
            ImGui::TableSetupColumn(
                "Shader",
                ImGuiTableColumnFlags_WidthStretch,
                0.68f);
            ImGui::TableSetupColumn(
                "Entry",
                ImGuiTableColumnFlags_WidthStretch,
                0.32f);
            ImGui::TableHeadersRow();

            for (ShaderAssetHandle shaderHandle : materialTemplate->shaders())
            {
                ImGui::PushID(static_cast<int>(shaderHandle.index));
                ImGui::TableNextRow();
                if (shaderHandle && assets.contains(shaderHandle))
                {
                    const ShaderAsset& shader = assets.shader(shaderHandle);
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(shaderStageName(shader.stage()));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(
                        displayName(shader.name(), "Unnamed Shader"));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(shader.entryPoint().c_str());
                }
                else
                {
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Unknown");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextDisabled("Missing Shader");
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader(
            "Pipeline State",
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        const MaterialRenderState& renderState = material.renderState();
        const PipelineVariantKey pipelineKey = makePipelineVariantKey(
            templateHandle,
            renderState);
        const RenderQueue queue = renderQueueFor(renderState);
        const std::string queueLabel =
            std::string(renderQueueName(queue)) + " (" +
            std::to_string(renderQueueValue(queue)) + ")";

        drawProperty(
            "Surface",
            renderState.transparent() ? "Transparent" : "Opaque");
        drawProperty("Render Queue", queueLabel.c_str());
        drawProperty("Blend", blendModeName(pipelineKey.blendMode));
        drawProperty("Cull", cullModeName(pipelineKey.cullMode));
        drawProperty(
            "Double Sided",
            renderState.doubleSided ? "Enabled" : "Disabled");
        drawProperty(
            "Depth Test",
            pipelineKey.depthTestEnabled ? "Enabled" : "Disabled");
        drawProperty(
            "Depth Write",
            pipelineKey.depthWriteEnabled ? "Enabled" : "Disabled");
        drawProperty(
            "Depth Compare",
            depthCompareName(pipelineKey.depthCompare));
        drawProperty(
            "Shader Feature",
            pipelineKey.shaderFeatures == ShaderFeatureFlags::AlphaClip
                ? "Alpha Clip"
                : "None");
        drawProperty(
            "Alpha Clip",
            renderState.alphaClipEnabled ? "Enabled" : "Disabled");
        if (renderState.alphaClipEnabled)
        {
            drawProperty("Alpha Threshold", renderState.alphaClipThreshold);
        }
    }

    if (!ImGui::CollapsingHeader(
            "Properties",
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        return navigation;
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##MaterialProperties", 3, tableFlags))
    {
        return navigation;
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
            drawTextureImage(
                texturePreviews,
                textureHandle,
                texture,
                thumbnailExtent,
                thumbnailExtent);
            ImGui::SameLine();
            ImGui::TextUnformatted(
                displayName(texture.name(), "Unnamed Texture"));
            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton("Inspect"))
            {
                navigation = InspectorTarget{textureHandle};
            }
        }
        else
        {
            ImGui::TextDisabled("None");
        }
        ImGui::PopID();
    }

    ImGui::EndTable();
    return navigation;
}

std::optional<InspectorTarget> MaterialInspector::drawMaterialTemplate(
    const AssetManager& assets,
    MaterialTemplateAssetHandle target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled(
            "MaterialTemplate selection is no longer valid");
        return std::nullopt;
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
    return std::nullopt;
}

} // namespace VkRenderer
