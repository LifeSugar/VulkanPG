#pragma once

#include "Asset/AssetManager.h"
#include "GLBTypes.h"

#include <filesystem>
#include <functional>
#include <vector>

namespace VkRenderer
{

/// Converts GLB texture payloads into source-independent TextureAssets.
class GLBTextureImporter final
{
public:
    using Decoder = std::function<TextureAsset::CreateInfo(
        const GLBTexture&,
        const std::filesystem::path& baseDirectory)>;

    struct CreateInfo
    {
        AssetManager* assets = nullptr;
        std::filesystem::path baseDirectory;
        TextureColorSpace colorSpace = TextureColorSpace::Srgb;
        TextureSamplerDesc sampler;
        TextureAssetHandle fallbackTexture;
        Decoder decoder;
    };

    [[nodiscard]] std::vector<TextureAssetHandle> import(
        const std::vector<GLBTexture>& source,
        const CreateInfo& createInfo) const;
};

} // namespace VkRenderer
