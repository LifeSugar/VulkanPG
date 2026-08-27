#pragma once

#include "asset/AssetManager.hpp"
#include "gltf/GLBTypes.hpp"

#include <string>
#include <vector>

namespace rubia::importer::gltf
{

/// Maps GLB material semantics onto one caller-selected material template.
struct GLBMaterialMapping
{
    asset::MaterialTemplateAssetHandle materialTemplate;

    std::string baseColorParameter;
    std::string metallicParameter;
    std::string roughnessParameter;
    std::string emissiveParameter;

    std::string baseColorTextureSlot;
    std::string metallicRoughnessTextureSlot;
    std::string normalTextureSlot;
    std::string occlusionTextureSlot;
    std::string emissiveTextureSlot;
};

class GLBMaterialImporter final
{
public:
    struct CreateInfo
    {
        asset::AssetManager* assets = nullptr;
        GLBMaterialMapping mapping;
        const std::vector<asset::TextureAssetHandle>* textures = nullptr;
        asset::TextureAssetHandle defaultTexture;
        asset::TextureAssetHandle defaultDataTexture;
        asset::TextureAssetHandle defaultNormalTexture;
    };

    [[nodiscard]] std::vector<asset::MaterialAssetHandle> import(
        const std::vector<GLBMaterial>& source,
        const CreateInfo& createInfo) const;
};

} // namespace rubia::importer::gltf
