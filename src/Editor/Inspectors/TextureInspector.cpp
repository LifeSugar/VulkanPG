#include "Editor/Inspectors/TextureInspector.h"

#include "Asset/AssetManager.h"
#include "Editor/EditorTexturePreview.h"
#include "Editor/Inspectors/InspectorWidgets.h"

#include <imgui.h>

namespace VkRenderer
{

namespace
{

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

void TextureInspector::draw(
    const AssetManager& assets,
    EditorTexturePreviewProvider& texturePreviews,
    TextureAssetHandle target) const
{
    using namespace InspectorWidgets;

    if (!assets.contains(target))
    {
        ImGui::TextDisabled("TextureAsset selection is no longer valid");
        return;
    }

    const TextureAsset& texture = assets.texture(target);
    ImGui::SeparatorText("Texture Asset");
    drawProperty("Name", displayName(texture.name(), "Unnamed Texture"));

    ImGui::SeparatorText("Preview");
    drawTextureImage(
        texturePreviews,
        target,
        texture,
        ImGui::GetContentRegionAvail().x,
        320.0f);

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

} // namespace VkRenderer
