// KTX2 format code values are adapted from the generated
// KhronosGroup/KTX-Software lib/vkformat_enum.h v4.4.2.
// Copyright 2015-2024 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "texture/Ktx2Format.hpp"

#include <stdexcept>

namespace VkRenderer
{

Ktx2FormatCode textureKtx2Format(
    TextureFormat format,
    TextureColorSpace colorSpace)
{
    const bool srgb = colorSpace == TextureColorSpace::Srgb;
    switch (format)
    {
    case TextureFormat::R8UNorm:
        return srgb ? Ktx2FormatCode::R8Srgb : Ktx2FormatCode::R8UNorm;
    case TextureFormat::RG8UNorm:
        return srgb ? Ktx2FormatCode::RG8Srgb : Ktx2FormatCode::RG8UNorm;
    case TextureFormat::RGBA8UNorm:
        return srgb ? Ktx2FormatCode::RGBA8Srgb : Ktx2FormatCode::RGBA8UNorm;
    case TextureFormat::RGBA16Float:
        if (!srgb) return Ktx2FormatCode::RGBA16Float;
        break;
    case TextureFormat::RGBA32Float:
        if (!srgb) return Ktx2FormatCode::RGBA32Float;
        break;
    case TextureFormat::BC1RGBUNorm:
        return srgb ? Ktx2FormatCode::BC1RGBSrgb : Ktx2FormatCode::BC1RGBUNorm;
    case TextureFormat::BC1RGBAUNorm:
        return srgb ? Ktx2FormatCode::BC1RGBASrgb : Ktx2FormatCode::BC1RGBAUNorm;
    case TextureFormat::BC2UNorm:
        return srgb ? Ktx2FormatCode::BC2Srgb : Ktx2FormatCode::BC2UNorm;
    case TextureFormat::BC3UNorm:
        return srgb ? Ktx2FormatCode::BC3Srgb : Ktx2FormatCode::BC3UNorm;
    case TextureFormat::BC4UNorm:
        if (!srgb) return Ktx2FormatCode::BC4UNorm;
        break;
    case TextureFormat::BC4SNorm:
        if (!srgb) return Ktx2FormatCode::BC4SNorm;
        break;
    case TextureFormat::BC5UNorm:
        if (!srgb) return Ktx2FormatCode::BC5UNorm;
        break;
    case TextureFormat::BC5SNorm:
        if (!srgb) return Ktx2FormatCode::BC5SNorm;
        break;
    case TextureFormat::BC6HUFloat:
        if (!srgb) return Ktx2FormatCode::BC6HUFloat;
        break;
    case TextureFormat::BC6HSFloat:
        if (!srgb) return Ktx2FormatCode::BC6HSFloat;
        break;
    case TextureFormat::BC7UNorm:
        return srgb ? Ktx2FormatCode::BC7Srgb : Ktx2FormatCode::BC7UNorm;
    case TextureFormat::Undefined:
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
        return Ktx2TextureFormatMapping{TextureFormat::R8UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::R8Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::R8UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::RG8UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::RG8UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::RG8Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::RG8UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::RGBA8UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::RGBA8UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::RGBA8Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::RGBA8UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::RGBA16Float:
        return Ktx2TextureFormatMapping{TextureFormat::RGBA16Float, TextureColorSpace::Linear};
    case Ktx2FormatCode::RGBA32Float:
        return Ktx2TextureFormatMapping{TextureFormat::RGBA32Float, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBUNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC1RGBUNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBSrgb:
        return Ktx2TextureFormatMapping{TextureFormat::BC1RGBUNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC1RGBAUNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC1RGBAUNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC1RGBASrgb:
        return Ktx2TextureFormatMapping{TextureFormat::BC1RGBAUNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC2UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC2UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC2Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::BC2UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC3UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC3UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC3Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::BC3UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::BC4UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC4UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC4SNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC4SNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC5UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC5UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC5SNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC5SNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC6HUFloat:
        return Ktx2TextureFormatMapping{TextureFormat::BC6HUFloat, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC6HSFloat:
        return Ktx2TextureFormatMapping{TextureFormat::BC6HSFloat, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC7UNorm:
        return Ktx2TextureFormatMapping{TextureFormat::BC7UNorm, TextureColorSpace::Linear};
    case Ktx2FormatCode::BC7Srgb:
        return Ktx2TextureFormatMapping{TextureFormat::BC7UNorm, TextureColorSpace::Srgb};
    case Ktx2FormatCode::Undefined:
        break;
    }
    return std::nullopt;
}

} // namespace VkRenderer
