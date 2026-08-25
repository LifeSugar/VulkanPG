#pragma once

#include "Editor/EditorSelection.h"

#include <optional>

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;

class MaterialInspector final
{
public:
    [[nodiscard]] std::optional<InspectorTarget> drawMaterialAsset(
        const AssetManager& assets,
        ApplicationGuiRenderBridge& texturePreviews,
        MaterialAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawMaterialTemplate(
        const AssetManager& assets,
        MaterialTemplateAssetHandle target) const;
};

} // namespace VkRenderer
