#pragma once

#include "EditorFwd.hpp"
#include "EditorSelection.hpp"
#include "texture/TextureImportRegistry.hpp"

#include <vector>

namespace rubia::editor
{

/// Lists the runtime asset registry and emits editor import operations.
class AssetBrowserPanel final
{
public:
    [[nodiscard]] std::vector<importer::texture::TextureReimportRequest> draw(
        const asset::AssetManager& assets,
        const importer::texture::TextureImportRegistry* textureImports,
        EditorSelection& selection,
        bool* open = nullptr) const;
};

} // namespace rubia::editor
