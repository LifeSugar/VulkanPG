#include "Vulkan/TextureVkFormat.h"

#include <stdexcept>

namespace VkRenderer
{

VkFormat textureVkFormat(
    TextureFormat format,
    TextureColorSpace colorSpace)
{
    const bool srgb = colorSpace == TextureColorSpace::Srgb;
    switch (format)
    {
    case TextureFormat::R8UNorm:
        return srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
    case TextureFormat::RG8UNorm:
        return srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
    case TextureFormat::RGBA8UNorm:
        return srgb
            ? VK_FORMAT_R8G8B8A8_SRGB
            : VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::RGBA16Float:
        if (!srgb) return VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    case TextureFormat::RGBA32Float:
        if (!srgb) return VK_FORMAT_R32G32B32A32_SFLOAT;
        break;
    case TextureFormat::BC1RGBUNorm:
        return srgb
            ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
            : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case TextureFormat::BC1RGBAUNorm:
        return srgb
            ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK
            : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case TextureFormat::BC2UNorm:
        return srgb
            ? VK_FORMAT_BC2_SRGB_BLOCK
            : VK_FORMAT_BC2_UNORM_BLOCK;
    case TextureFormat::BC3UNorm:
        return srgb
            ? VK_FORMAT_BC3_SRGB_BLOCK
            : VK_FORMAT_BC3_UNORM_BLOCK;
    case TextureFormat::BC4UNorm:
        if (!srgb) return VK_FORMAT_BC4_UNORM_BLOCK;
        break;
    case TextureFormat::BC4SNorm:
        if (!srgb) return VK_FORMAT_BC4_SNORM_BLOCK;
        break;
    case TextureFormat::BC5UNorm:
        if (!srgb) return VK_FORMAT_BC5_UNORM_BLOCK;
        break;
    case TextureFormat::BC5SNorm:
        if (!srgb) return VK_FORMAT_BC5_SNORM_BLOCK;
        break;
    case TextureFormat::BC6HUFloat:
        if (!srgb) return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        break;
    case TextureFormat::BC6HSFloat:
        if (!srgb) return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        break;
    case TextureFormat::BC7UNorm:
        return srgb
            ? VK_FORMAT_BC7_SRGB_BLOCK
            : VK_FORMAT_BC7_UNORM_BLOCK;
    case TextureFormat::Undefined:
        break;
    }
    throw std::invalid_argument(
        "texture format and color space have no Vulkan representation");
}

std::optional<TextureFormatMapping>
textureFormatFromVk(VkFormat format) noexcept
{
    switch (format)
    {
    case VK_FORMAT_R8_UNORM:
        return TextureFormatMapping{TextureFormat::R8UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_R8_SRGB:
        return TextureFormatMapping{TextureFormat::R8UNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_R8G8_UNORM:
        return TextureFormatMapping{TextureFormat::RG8UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_R8G8_SRGB:
        return TextureFormatMapping{TextureFormat::RG8UNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_R8G8B8A8_UNORM:
        return TextureFormatMapping{TextureFormat::RGBA8UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_R8G8B8A8_SRGB:
        return TextureFormatMapping{TextureFormat::RGBA8UNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return TextureFormatMapping{TextureFormat::RGBA16Float, TextureColorSpace::Linear};
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return TextureFormatMapping{TextureFormat::RGBA32Float, TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC1RGBUNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return TextureFormatMapping{TextureFormat::BC1RGBUNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC1RGBAUNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return TextureFormatMapping{TextureFormat::BC1RGBAUNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC2UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return TextureFormatMapping{TextureFormat::BC2UNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC3UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return TextureFormatMapping{TextureFormat::BC3UNorm, TextureColorSpace::Srgb};
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC4UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC4SNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC5UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC5SNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return TextureFormatMapping{TextureFormat::BC6HUFloat, TextureColorSpace::Linear};
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return TextureFormatMapping{TextureFormat::BC6HSFloat, TextureColorSpace::Linear};
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return TextureFormatMapping{TextureFormat::BC7UNorm, TextureColorSpace::Linear};
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return TextureFormatMapping{TextureFormat::BC7UNorm, TextureColorSpace::Srgb};
    default:
        return std::nullopt;
    }
}

} // namespace VkRenderer
