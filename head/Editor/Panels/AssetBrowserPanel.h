#pragma once

#include "Editor/EditorSelection.h"
#include "Import/TextureImportRegistry.h"

#include <vector>

namespace VkRenderer
{

class AssetManager;

/// Lists the runtime asset registry and emits editor import operations.
class AssetBrowserPanel final
{
public:
    [[nodiscard]] std::vector<TextureReimportRequest> draw(
        const AssetManager& assets,
        const TextureImportRegistry* textureImports,
        EditorSelection& selection,
        bool* open = nullptr) const;
};

} // namespace VkRenderer
