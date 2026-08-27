// KTX2 format code values are adapted from the generated
// KhronosGroup/KTX-Software lib/vkformat_enum.h v4.4.2.
// Copyright 2015-2024 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "texture/Ktx2Format.hpp"

#include <stdexcept>

namespace rubia::importer::texture
{

Ktx2FormatCode textureKtx2Format(
    asset::TextureFormat format,
    asset::TextureColorSpace colorSpace)
{
    const bool srgb = colorSpace == asset::TextureColorSpace::Srgb;
    switch (format)
    {
    case asset::TextureFormat::R8UNorm:
        return srgb ? Ktx2FormatCode::R8Srgb : Ktx2FormatCode::R8UNorm;
    case asset::TextureFormat::RG8UNorm:
        return srgb ? Ktx2FormatCode::RG8Srgb : Ktx2FormatCode::RG8UNorm;
    case asset::TextureFormat::RGBA8UNorm:
        return srgb ? Ktx2FormatCode::RGBA8Srgb : Ktx2FormatCode::RGBA8UNorm;
    case asset::TextureFormat::RGBA16Float:
        if (!srgb) return Ktx2FormatCode::RGBA16Float;
        break;
    case asset::TextureFormat::RGBA32Float:
        if (!srgb) return Ktx2FormatCode::RGBA32Float;
        break;
    case asset::TextureFormat::BC1RGBUNorm:
        return srgb ? Ktx2FormatCode::BC1RGBSrgb : Ktx2FormatCode::BC1RGBUNorm;
    case asset::TextureFormat::BC1RGBAUNorm:
        return srgb ? Ktx2FormatCode::BC1RGBASrgb : Ktx2FormatCode::BC1RGBAUNorm;
    case asset::TextureFormat::BC2UNorm:
        return srgb ? Ktx2FormatCode::BC2Srgb : Ktx2FormatCode::BC2UNorm;
    case asset::TextureFormat::BC3UNorm:
        return srgb ? Ktx2FormatCode::BC3Srgb : Ktx2FormatCode::BC3UNorm;
    case asset::TextureFormat::BC4UNorm:
        if (!srgb) return Ktx2FormatCode::BC4UNorm;
        break;
    case asset::TextureFormat::BC4SNorm:
        if (!srgb) return Ktx2FormatCode::BC4SNorm;
        break;
    case asset::TextureFormat::BC5UNorm:
        if (!srgb) return Ktx2FormatCode::BC5UNorm;
        break;
    case asset::TextureFormat::BC5SNorm:
        if (!srgb) return Ktx2FormatCode::BC5SNorm;
        break;
    case asset::TextureFormat::BC6HUFloat:
        if (!srgb) return Ktx2FormatCode::BC6HUFloat;
        break;
    case asset::TextureFormat::BC6HSFloat:
        if (!srgb) return Ktx2FormatCode::BC6HSFloat;
        break;
    case asset::TextureFormat::BC7UNorm:
        return srgb ? Ktx2FormatCode::BC7Srgb : Ktx2FormatCode::BC7UNorm;
    case asset::TextureFormat::Undefined:
        break;
    }
    throw std::invalid_argument(
        "texture format and transfer function have no KTX2 representation");
}

std::optional<Ktx2TextureFormatMapping>
textureFormatFromKtx2(uint32_t formatCode) noexcept
{
    switch (static_cast<Ktx2FormatCode>(formatCode))
    {
    case Ktx2FormatCode::R8UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::R8UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::R8Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::R8UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::RG8UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RG8UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::RG8Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RG8UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::RGBA8UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RGBA8UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::RGBA8Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RGBA8UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::RGBA16Float:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RGBA16Float, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::RGBA32Float:
        return Ktx2TextureFormatMapping{asset::TextureFormat::RGBA32Float, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBUNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC1RGBUNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBSrgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC1RGBUNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC1RGBAUNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC1RGBAUNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBASrgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC1RGBAUNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC2UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC2UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC2Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC2UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC3UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC3UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC3Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC3UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC4UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC4UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC4SNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC4SNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC5UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC5UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC5SNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC5SNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC6HUFloat:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC6HUFloat, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC6HSFloat:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC6HSFloat, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC7UNorm:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC7UNorm, asset::TextureColorSpace::Linear};
    case Ktx2FormatCode::BC7Srgb:
        return Ktx2TextureFormatMapping{asset::TextureFormat::BC7UNorm, asset::TextureColorSpace::Srgb};
    case Ktx2FormatCode::Undefined:
        break;
    }
    return std::nullopt;
}

} // namespace rubia::importer::texture
