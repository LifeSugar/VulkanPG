#pragma once

#include "Asset/AssetFwd.h"

#include <cstdint>
#include <string>

namespace VkRenderer
{

class EditorTexturePreviewProvider;
class TextureAsset;

namespace InspectorWidgets
{

[[nodiscard]] const char* displayName(
    const std::string& name,
    const char* fallback) noexcept;

void drawProperty(const char* label, const char* value);
void drawProperty(const char* label, uint32_t value);
void drawProperty(const char* label, float value);

[[nodiscard]] bool drawReference(
    const char* label,
    const char* value,
    int id);

void drawTextureImage(
    EditorTexturePreviewProvider& texturePreviews,
    TextureAssetHandle handle,
    const TextureAsset& texture,
    float maxWidth,
    float maxHeight);

} // namespace InspectorWidgets

} // namespace VkRenderer
