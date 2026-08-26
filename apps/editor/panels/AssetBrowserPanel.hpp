#pragma once

#include "EditorSelection.hpp"
#include "texture/TextureImportRegistry.hpp"

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
