// Texture construction and Basis/Zstd sequencing are adapted from
// KhronosGroup/KTX-Software tools/toktx/toktx.cc and utils/scapp.h v4.4.2.
// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "texture/Ktx2ContainerWriter.hpp"

#include "texture/Ktx2Format.hpp"

#include <ktx.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

namespace rubia::importer::texture
{
namespace
{

using KtxTexture =
    std::unique_ptr<ktxTexture2, decltype(&ktxTexture2_Destroy)>;

[[noreturn]] void throwKtxError(
    const char* operation,
    KTX_error_code error)
{
    throw std::runtime_error(
        std::string(operation) + ": " + ktxErrorString(error));
}

[[nodiscard]] ktx_pack_uastc_flags uastcFlags(uint32_t qualityLevel)
{
    return static_cast<ktx_pack_uastc_flags>(qualityLevel);
}

[[nodiscard]] KtxTexture createTexture(
    const Ktx2ContainerWriteInfo& writeInfo)
{
    if (writeInfo.levels.empty())
    {
        throw std::invalid_argument("KTX2 container requires image levels");
    }

    ktxTextureCreateInfo createInfo{};
    createInfo.vkFormat = static_cast<uint32_t>(textureKtx2Format(
        writeInfo.sourceFormat,
        writeInfo.transferFunction));
    createInfo.baseWidth = writeInfo.levels.front().width;
    createInfo.baseHeight = writeInfo.levels.front().height;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = static_cast<uint32_t>(writeInfo.levels.size());
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;

    ktxTexture2* rawTexture = nullptr;
    const KTX_error_code createResult = ktxTexture2_Create(
        &createInfo,
        KTX_TEXTURE_CREATE_ALLOC_STORAGE,
        &rawTexture);
    if (createResult != KTX_SUCCESS)
    {
        throwKtxError("failed to create the cooked KTX2 texture", createResult);
    }
    KtxTexture texture(rawTexture, &ktxTexture2_Destroy);

    for (uint32_t level = 0;
         level < static_cast<uint32_t>(writeInfo.levels.size());
         ++level)
    {
        const Ktx2ImageLevel& mip = writeInfo.levels[level];
        const KTX_error_code setResult = ktxTexture_SetImageFromMemory(
            ktxTexture(texture.get()),
            level,
            0,
            0,
            reinterpret_cast<const ktx_uint8_t*>(mip.payload.data()),
            mip.payload.size());
        if (setResult != KTX_SUCCESS)
        {
            throwKtxError("failed to populate a cooked KTX2 mip", setResult);
        }
    }
    return texture;
}

void encodeBasis(
    ktxTexture2& texture,
    const KtxBasisEncodeSettings& settings)
{
    if (settings.encoding == KtxPayloadEncoding::Uncompressed)
    {
        return;
    }

    ktxBasisParams parameters{};
    parameters.structSize = sizeof(parameters);
    parameters.uastc = settings.encoding == KtxPayloadEncoding::Uastc;
    parameters.threadCount = settings.threadCount;
    parameters.compressionLevel = settings.etc1sCompressionLevel;
    parameters.qualityLevel = settings.etc1sQualityLevel;
    parameters.normalMap = settings.normalMap;
    parameters.uastcFlags = uastcFlags(settings.uastcQualityLevel);
    parameters.uastcRDO = settings.uastcRdo;
    parameters.uastcRDOQualityScalar = settings.uastcRdoQualityScalar;
    parameters.uastcRDODictSize = settings.uastcRdoDictionarySize;
    if (!settings.inputSwizzle.empty())
    {
        std::copy_n(
            settings.inputSwizzle.data(),
            4,
            parameters.inputSwizzle);
    }

    const KTX_error_code encodeResult =
        ktxTexture2_CompressBasisEx(&texture, &parameters);
    if (encodeResult != KTX_SUCCESS)
    {
        throwKtxError("failed to Basis-encode the cooked KTX2", encodeResult);
    }
}

} // namespace

std::filesystem::path writeKtx2Container(
    const Ktx2ContainerWriteInfo& writeInfo)
{
    KtxTexture texture = createTexture(writeInfo);
    encodeBasis(*texture, writeInfo.basis);
    if (writeInfo.zstdLevel != 0)
    {
        const KTX_error_code zstdResult =
            ktxTexture2_DeflateZstd(texture.get(), writeInfo.zstdLevel);
        if (zstdResult != KTX_SUCCESS)
        {
            throwKtxError("failed to Zstd-compress the cooked KTX2", zstdResult);
        }
    }

    const std::filesystem::path outputPath =
        std::filesystem::absolute(writeInfo.outputPath).lexically_normal();
    if (!outputPath.parent_path().empty())
    {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    const std::string nativePath = outputPath.u8string();
    const KTX_error_code writeResult =
        ktxTexture2_WriteToNamedFile(texture.get(), nativePath.c_str());
    if (writeResult != KTX_SUCCESS)
    {
        throwKtxError("failed to write the cooked KTX2 file", writeResult);
    }
    return outputPath;
}

} // namespace rubia::importer::texture
