#pragma once

#include "asset/TextureAsset.hpp"
#include "texture/KtxTextureImporter.hpp"
#include "texture/TextureImportSettings.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace VkRenderer
{

/// Cross-platform, in-process equivalent of the PNG/JPG -> KTX2 part of
/// toktx. It composes stb image processing with libktx container/encoding APIs.
class KtxTextureCooker final
{
public:
    struct Request
    {
        std::filesystem::path inputPath;
        /// Persistent local KTX2 output. The cooker never removes this file.
        std::filesystem::path outputPath;
        TextureColorSpace colorSpace = TextureColorSpace::Linear;
        bool generateMipmaps = false;
        TextureMipFilter mipFilter = TextureMipFilter::Mitchell;
        TextureMipEdgeMode mipEdgeMode = TextureMipEdgeMode::Clamp;
        KtxBasisEncodeSettings basis;
        /// Zero disables Zstd; otherwise valid libktx levels are 1-22.
        uint32_t zstdLevel = 0;
    };

    struct Result
    {
        std::filesystem::path outputPath;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevelCount = 0;
    };

    [[nodiscard]] Result cookToFile(const Request& request) const;

    [[nodiscard]] TextureAsset::CreateInfo cookAndImport(
        const Request& request,
        const KtxTextureImporter::CreateInfo& importInfo) const;
};

} // namespace VkRenderer
