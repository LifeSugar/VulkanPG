#pragma once

#include "Asset/AssetManager.h"
#include "Import/GLBMaterialImporter.h"
#include "Import/GLBTextureImporter.h"

#include <filesystem>
#include <vector>

struct GLBModel;

namespace VkRenderer
{

/// Orchestrates the GLB-specific importers and creates one ModelAsset.
class GLBModelImporter final
{
public:
    struct CreateInfo
    {
        AssetManager* assets = nullptr;

        std::filesystem::path baseDirectory;
        TextureColorSpace textureColorSpace = TextureColorSpace::Srgb;
        TextureSamplerDesc textureSampler;
        TextureAssetHandle defaultTexture;
        GLBTextureImporter::Decoder textureDecoder;

        GLBMaterialMapping materialMapping;
        MaterialAssetHandle fallbackMaterial;
    };

    struct Result
    {
        ModelAssetHandle model;
        std::vector<TextureAssetHandle> textures;
        std::vector<MaterialAssetHandle> materials;
        std::vector<MeshAssetHandle> meshes;
    };

    [[nodiscard]] Result import(
        const GLBModel& source,
        const CreateInfo& createInfo) const;
};

} // namespace VkRenderer
