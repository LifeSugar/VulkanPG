#pragma once

#include "asset/AssetManager.hpp"
#include "gltf/GLBTypes.hpp"

#include <vector>

namespace rubia::importer::gltf
{

/// Converts GLB geometry into source-independent MeshAssets.
class GLBMeshImporter final
{
public:
    struct CreateInfo
    {
        asset::AssetManager* assets = nullptr;
        const std::vector<asset::MaterialAssetHandle>* materials = nullptr;
        asset::MaterialAssetHandle fallbackMaterial;
    };

    [[nodiscard]] std::vector<asset::MeshAssetHandle> import(
        const std::vector<GLBMesh>& source,
        const CreateInfo& createInfo) const;
};

} // namespace rubia::importer::gltf
