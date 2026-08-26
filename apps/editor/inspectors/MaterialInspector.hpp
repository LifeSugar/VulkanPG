#pragma once

#include "EditorSelection.hpp"
#include "texture/TextureImportRegistry.hpp"

#include <optional>
#include <vector>

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;

struct MaterialInspectorOutput
{
    std::optional<InspectorTarget> navigation;
    std::vector<TextureReimportRequest> textureReimports;
};

class MaterialInspector final
{
public:
    [[nodiscard]] MaterialInspectorOutput drawMaterialAsset(
        const AssetManager& assets,
        ApplicationGuiRenderBridge& texturePreviews,
        const TextureImportRegistry* textureImports,
        MaterialAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawMaterialTemplate(
        const AssetManager& assets,
        MaterialTemplateAssetHandle target) const;
};

} // namespace VkRenderer
