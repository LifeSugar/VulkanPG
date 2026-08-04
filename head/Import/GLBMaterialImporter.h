#pragma once

#include "Asset/AssetManager.h"
#include "GLBTypes.h"

#include <string>
#include <vector>

namespace VkRenderer
{

/// Maps GLB material semantics onto one caller-selected material template.
struct GLBMaterialMapping
{
    MaterialTemplateAssetHandle materialTemplate;

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
        AssetManager* assets = nullptr;
        GLBMaterialMapping mapping;
        const std::vector<TextureAssetHandle>* textures = nullptr;
        TextureAssetHandle defaultTexture;
    };

    [[nodiscard]] std::vector<MaterialAssetHandle> import(
        const std::vector<GLBMaterial>& source,
        const CreateInfo& createInfo) const;
};

} // namespace VkRenderer
