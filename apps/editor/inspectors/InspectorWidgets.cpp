#include "inspectors/InspectorWidgets.hpp"

#include "asset/TextureAsset.hpp"
#include "render/ApplicationGuiRenderBridge.hpp"

#include <imgui.h>

namespace VkRenderer::InspectorWidgets
{

const char* displayName(
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

void drawProperty(const char* label, float value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(120.0f);
    ImGui::Text("%.3f", value);
}

bool drawReference(const char* label, const char* value, int id)
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

void drawTextureImage(
    ApplicationGuiRenderBridge& texturePreviews,
    TextureAssetHandle handle,
    const TextureAsset& texture,
    float maxWidth,
    float maxHeight)
{
    if (texture.width() == 0 || texture.height() == 0 ||
        maxWidth <= 0.0f || maxHeight <= 0.0f)
    {
        ImGui::TextDisabled("Preview unavailable");
        return;
    }

    const ApplicationGuiTexture preview = texturePreviews.preview(handle);
    if (!preview)
    {
        ImGui::TextDisabled("Preview unavailable");
        return;
    }

    const float aspect =
        static_cast<float>(texture.width()) /
        static_cast<float>(texture.height());
    ImVec2 size{maxWidth, maxWidth / aspect};
    if (size.y > maxHeight)
    {
        size.y = maxHeight;
        size.x = maxHeight * aspect;
    }

    const ImTextureRef textureReference(
        static_cast<ImTextureID>(preview.textureId));
    ImGui::ImageWithBg(
        textureReference,
        size,
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f),
        ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
}

} // namespace VkRenderer::InspectorWidgets
