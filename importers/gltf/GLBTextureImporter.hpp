#pragma once

#include "asset/AssetManager.hpp"
#include "gltf/GLBTypes.hpp"

#include <filesystem>
#include <functional>
#include <vector>

namespace rubia::importer::gltf
{

/// Converts GLB texture payloads into source-independent TextureAssets.
class GLBTextureImporter final
{
public:
    using Decoder = std::function<asset::TextureAsset::CreateInfo(
        const GLBTexture&,
        const std::filesystem::path& baseDirectory)>;

    struct CreateInfo
    {
        asset::AssetManager* assets = nullptr;
        std::filesystem::path baseDirectory;
        asset::TextureColorSpace colorSpace = asset::TextureColorSpace::Srgb;
        /// Optional per-source override derived from material semantics.
        const std::vector<asset::TextureColorSpace>* colorSpaces = nullptr;
        asset::TextureSamplerDesc sampler;
        asset::TextureAssetHandle fallbackTexture;
        Decoder decoder;
    };

    [[nodiscard]] std::vector<asset::TextureAssetHandle> import(
        const std::vector<GLBTexture>& source,
        const CreateInfo& createInfo) const;
};

} // namespace rubia::importer::gltf
