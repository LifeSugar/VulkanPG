#pragma once

#include "Asset/AssetManager.h"
#include "GLBTypes.h"

#include <vector>

namespace VkRenderer
{

/// Converts GLB geometry into source-independent MeshAssets.
class GLBMeshImporter final
{
public:
    struct CreateInfo
    {
        AssetManager* assets = nullptr;
        const std::vector<MaterialAssetHandle>* materials = nullptr;
        MaterialAssetHandle fallbackMaterial;
    };

    [[nodiscard]] std::vector<MeshAssetHandle> import(
        const std::vector<GLBMesh>& source,
        const CreateInfo& createInfo) const;
};

} // namespace VkRenderer
