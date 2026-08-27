#pragma once

#include "asset/TextureAsset.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace rubia::importer::texture
{

/// Converts a KTX2 container into an engine-owned TextureAsset payload.
class KtxTextureImporter final
{
public:
    struct CreateInfo
    {
        std::string name;
        asset::TextureSamplerDesc sampler;
        /// Used only when a BasisLZ/UASTC payload requires transcoding.
        asset::TextureFormat transcodeFormat = asset::TextureFormat::BC7UNorm;
        bool highQuality = true;
    };

    [[nodiscard]] asset::TextureAsset::CreateInfo importMemory(
        const void* data,
        std::size_t size,
        const CreateInfo& createInfo) const;

    [[nodiscard]] asset::TextureAsset::CreateInfo importFile(
        const std::filesystem::path& path,
        const CreateInfo& createInfo) const;
};

} // namespace rubia::importer::texture
