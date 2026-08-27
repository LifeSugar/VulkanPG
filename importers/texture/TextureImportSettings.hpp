#pragma once

#include "asset/TextureAsset.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <thread>

namespace rubia::importer::texture
{

[[nodiscard]] inline uint32_t defaultTextureImportThreadCount() noexcept
{
    const uint32_t available = std::thread::hardware_concurrency();
    return std::max(1u, std::min(available, 64u));
}

enum class KtxPayloadEncoding
{
    Uncompressed,
    Etc1s,
    Uastc
};

enum class TextureMipFilter
{
    Box,
    Triangle,
    CubicBSpline,
    CatmullRom,
    Mitchell,
    Point
};

enum class TextureMipEdgeMode
{
    Clamp,
    Reflect,
    Wrap,
    Zero
};

/// User-selected KTX2 payload encoder parameters. No texture semantic is
/// inferred by this settings model or by its editor controls.
struct KtxBasisEncodeSettings
{
    KtxPayloadEncoding encoding = KtxPayloadEncoding::Uncompressed;
    uint32_t threadCount = defaultTextureImportThreadCount();
    std::string inputSwizzle;
    bool normalMap = false;

    uint32_t etc1sCompressionLevel = 2;
    uint32_t etc1sQualityLevel = 128;

    uint32_t uastcQualityLevel = 2;
    bool uastcRdo = false;
    float uastcRdoQualityScalar = 1.0f;
    uint32_t uastcRdoDictionarySize = 4096;
};

/// Complete, renderer-independent settings used to cook and import one
/// texture. The Vulkan backend only sees the resulting TextureAsset.
struct TextureImportSettings
{
    asset::TextureColorSpace colorSpace = asset::TextureColorSpace::Linear;
    bool generateMipmaps = false;
    TextureMipFilter mipFilter = TextureMipFilter::Mitchell;
    TextureMipEdgeMode mipEdgeMode = TextureMipEdgeMode::Clamp;
    KtxBasisEncodeSettings basis;
    uint32_t zstdLevel = 0;

    /// Used only when a Basis payload is imported for the runtime.
    asset::TextureFormat transcodeFormat = asset::TextureFormat::BC7UNorm;
    bool highQualityTranscode = true;
};

} // namespace rubia::importer::texture
