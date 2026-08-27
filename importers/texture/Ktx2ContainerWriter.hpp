#pragma once

#include "asset/TextureAsset.hpp"
#include "texture/TextureImportSettings.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace rubia::importer::texture
{

struct Ktx2ImageLevel
{
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<std::byte> payload;
};

struct Ktx2ContainerWriteInfo
{
    std::filesystem::path outputPath;
    asset::TextureFormat sourceFormat = asset::TextureFormat::Undefined;
    asset::TextureColorSpace transferFunction = asset::TextureColorSpace::Linear;
    std::vector<Ktx2ImageLevel> levels;
    KtxBasisEncodeSettings basis;
    uint32_t zstdLevel = 0;
};

/// KTX2 serialization adapter. KTX2's format-code field is contained behind
/// this boundary and never becomes part of editor or engine asset metadata.
[[nodiscard]] std::filesystem::path writeKtx2Container(
    const Ktx2ContainerWriteInfo& writeInfo);

} // namespace rubia::importer::texture
