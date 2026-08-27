#include "texture/KtxTextureCooker.hpp"

#include "texture/Ktx2ContainerWriter.hpp"
#include "texture/StbImageDecoder.hpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rubia::importer::texture
{
namespace
{

[[nodiscard]] stbir_filter stbFilter(TextureMipFilter filter)
{
    switch (filter)
    {
    case TextureMipFilter::Box: return STBIR_FILTER_BOX;
    case TextureMipFilter::Triangle: return STBIR_FILTER_TRIANGLE;
    case TextureMipFilter::CubicBSpline: return STBIR_FILTER_CUBICBSPLINE;
    case TextureMipFilter::CatmullRom: return STBIR_FILTER_CATMULLROM;
    case TextureMipFilter::Mitchell: return STBIR_FILTER_MITCHELL;
    case TextureMipFilter::Point: return STBIR_FILTER_POINT_SAMPLE;
    }
    throw std::invalid_argument("unsupported texture mip filter");
}

[[nodiscard]] stbir_edge stbEdgeMode(TextureMipEdgeMode edgeMode)
{
    switch (edgeMode)
    {
    case TextureMipEdgeMode::Clamp: return STBIR_EDGE_CLAMP;
    case TextureMipEdgeMode::Reflect: return STBIR_EDGE_REFLECT;
    case TextureMipEdgeMode::Wrap: return STBIR_EDGE_WRAP;
    case TextureMipEdgeMode::Zero: return STBIR_EDGE_ZERO;
    }
    throw std::invalid_argument("unsupported texture mip edge mode");
}

void validateRequest(const KtxTextureCooker::Request& request)
{
    if (request.inputPath.empty() ||
        !std::filesystem::is_regular_file(request.inputPath))
    {
        throw std::invalid_argument(
            "KtxTextureCooker input is not a readable image file: " +
            request.inputPath.string());
    }
    if (request.outputPath.empty())
    {
        throw std::invalid_argument(
            "KtxTextureCooker requires a persistent local output path");
    }
    if (request.zstdLevel > 22)
    {
        throw std::invalid_argument("KTX2 Zstd level must be between 0 and 22");
    }
    if (request.basis.encoding == KtxPayloadEncoding::Etc1s &&
        request.zstdLevel != 0)
    {
        throw std::invalid_argument(
            "ETC1S/BasisLZ KTX2 cannot also use Zstd supercompression");
    }
    if (!request.basis.inputSwizzle.empty() &&
        request.basis.inputSwizzle.size() != 4)
    {
        throw std::invalid_argument(
            "Basis input swizzle must be empty or contain four components");
    }
    if (!request.basis.inputSwizzle.empty() &&
        request.basis.inputSwizzle.find_first_not_of("rgba01") !=
            std::string::npos)
    {
        throw std::invalid_argument(
            "Basis input swizzle components must be r, g, b, a, 0, or 1");
    }
    if (request.basis.normalMap &&
        request.colorSpace != asset::TextureColorSpace::Linear)
    {
        throw std::invalid_argument("normal-map encoding requires linear data");
    }
    if (request.basis.etc1sCompressionLevel > 6 ||
        request.basis.etc1sQualityLevel < 1 ||
        request.basis.etc1sQualityLevel > 255)
    {
        throw std::invalid_argument("ETC1S encoder settings are out of range");
    }
    if (request.basis.uastcQualityLevel > 4)
    {
        throw std::invalid_argument(
            "UASTC quality level must be between 0 and 4");
    }
    if (!std::isfinite(request.basis.uastcRdoQualityScalar) ||
        request.basis.uastcRdoQualityScalar < 0.001f ||
        request.basis.uastcRdoQualityScalar > 50.0f ||
        request.basis.uastcRdoDictionarySize < 64 ||
        request.basis.uastcRdoDictionarySize > 65536)
    {
        throw std::invalid_argument("UASTC RDO settings are out of range");
    }
}

[[nodiscard]] std::vector<Ktx2ImageLevel> buildMipChain(
    asset::TextureAsset::CreateInfo decoded,
    const KtxTextureCooker::Request& request)
{
    if (decoded.format != asset::TextureFormat::RGBA8UNorm ||
        decoded.width == 0 || decoded.height == 0 ||
        decoded.payload.size() != textureMipByteSize(
            asset::TextureFormat::RGBA8UNorm,
            decoded.width,
            decoded.height))
    {
        throw std::runtime_error(
            "KtxTextureCooker source decoder did not produce tight RGBA8 data");
    }

    std::vector<Ktx2ImageLevel> result;
    result.push_back({
        decoded.width,
        decoded.height,
        std::move(decoded.payload)});
    if (!request.generateMipmaps)
    {
        return result;
    }

    const uint32_t baseWidth = result.front().width;
    const uint32_t baseHeight = result.front().height;
    for (uint32_t level = 1;
         result.back().width > 1 || result.back().height > 1;
         ++level)
    {
        const Ktx2ImageLevel& source = result.front();
        Ktx2ImageLevel destination{};
        destination.width = std::max(
            1u,
            baseWidth >> std::min(level, 31u));
        destination.height = std::max(
            1u,
            baseHeight >> std::min(level, 31u));
        destination.payload.resize(textureMipByteSize(
            asset::TextureFormat::RGBA8UNorm,
            destination.width,
            destination.height));

        if (source.width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
            source.height > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
            destination.width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
            destination.height > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        {
            throw std::overflow_error("texture dimensions exceed stb_image_resize limits");
        }

        STBIR_RESIZE resize{};
        stbir_resize_init(
            &resize,
            source.payload.data(),
            static_cast<int>(source.width),
            static_cast<int>(source.height),
            0,
            destination.payload.data(),
            static_cast<int>(destination.width),
            static_cast<int>(destination.height),
            0,
            STBIR_RGBA,
            request.colorSpace == asset::TextureColorSpace::Srgb
                ? STBIR_TYPE_UINT8_SRGB
                : STBIR_TYPE_UINT8);
        if (!stbir_set_filters(
                &resize,
                stbFilter(request.mipFilter),
                stbFilter(request.mipFilter)) ||
            !stbir_set_edgemodes(
                &resize,
                stbEdgeMode(request.mipEdgeMode),
                stbEdgeMode(request.mipEdgeMode)) ||
            !stbir_resize_extended(&resize))
        {
            throw std::runtime_error("failed to generate texture mip level");
        }
        result.push_back(std::move(destination));
    }
    return result;
}

} // namespace

KtxTextureCooker::Result KtxTextureCooker::cookToFile(
    const Request& request) const
{
    validateRequest(request);
    asset::TextureAsset::CreateInfo decoded =
        StbImageDecoder{}.decodeFile(request.inputPath);
    std::vector<Ktx2ImageLevel> mipLevels =
        buildMipChain(std::move(decoded), request);
    Ktx2ContainerWriteInfo writeInfo{};
    writeInfo.outputPath = request.outputPath;
    writeInfo.sourceFormat = asset::TextureFormat::RGBA8UNorm;
    writeInfo.transferFunction = request.colorSpace;
    writeInfo.levels = std::move(mipLevels);
    writeInfo.basis = request.basis;
    writeInfo.zstdLevel = request.zstdLevel;

    const uint32_t width = writeInfo.levels.front().width;
    const uint32_t height = writeInfo.levels.front().height;
    const uint32_t mipLevelCount =
        static_cast<uint32_t>(writeInfo.levels.size());
    const std::filesystem::path outputPath = writeKtx2Container(writeInfo);

    return {
        outputPath,
        width,
        height,
        mipLevelCount};
}

asset::TextureAsset::CreateInfo KtxTextureCooker::cookAndImport(
    const Request& request,
    const KtxTextureImporter::CreateInfo& importInfo) const
{
    const Result cooked = cookToFile(request);
    return KtxTextureImporter{}.importFile(cooked.outputPath, importInfo);
}

} // namespace rubia::importer::texture
