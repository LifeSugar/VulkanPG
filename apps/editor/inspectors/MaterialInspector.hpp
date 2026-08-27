#pragma once

#include "EditorFwd.hpp"
#include "EditorSelection.hpp"
#include "texture/TextureImportRegistry.hpp"

#include <optional>
#include <vector>

namespace rubia::editor
{

struct MaterialInspectorOutput
{
    std::optional<InspectorTarget> navigation;
    std::vector<importer::texture::TextureReimportRequest> textureReimports;
};

class MaterialInspector final
{
public:
    [[nodiscard]] MaterialInspectorOutput drawMaterialAsset(
        const asset::AssetManager& assets,
        render::ApplicationGuiRenderBridge& texturePreviews,
        const importer::texture::TextureImportRegistry* textureImports,
        asset::MaterialAssetHandle target) const;
    [[nodiscard]] std::optional<InspectorTarget> drawMaterialTemplate(
        const asset::AssetManager& assets,
        asset::MaterialTemplateAssetHandle target) const;
};

} // namespace rubia::editor
