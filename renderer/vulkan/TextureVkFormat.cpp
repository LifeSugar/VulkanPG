#include "vulkan/TextureVkFormat.hpp"

#include <stdexcept>

namespace rubia::rhi::vulkan
{

VkFormat textureVkFormat(
    asset::TextureFormat format,
    asset::TextureColorSpace colorSpace)
{
    const bool srgb = colorSpace == asset::TextureColorSpace::Srgb;
    switch (format)
    {
    case asset::TextureFormat::R8UNorm:
        return srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
    case asset::TextureFormat::RG8UNorm:
        return srgb ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
    case asset::TextureFormat::RGBA8UNorm:
        return srgb
            ? VK_FORMAT_R8G8B8A8_SRGB
            : VK_FORMAT_R8G8B8A8_UNORM;
    case asset::TextureFormat::RGBA16Float:
        if (!srgb) return VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    case asset::TextureFormat::RGBA32Float:
        if (!srgb) return VK_FORMAT_R32G32B32A32_SFLOAT;
        break;
    case asset::TextureFormat::BC1RGBUNorm:
        return srgb
            ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
            : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case asset::TextureFormat::BC1RGBAUNorm:
        return srgb
            ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK
            : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case asset::TextureFormat::BC2UNorm:
        return srgb
            ? VK_FORMAT_BC2_SRGB_BLOCK
            : VK_FORMAT_BC2_UNORM_BLOCK;
    case asset::TextureFormat::BC3UNorm:
        return srgb
            ? VK_FORMAT_BC3_SRGB_BLOCK
            : VK_FORMAT_BC3_UNORM_BLOCK;
    case asset::TextureFormat::BC4UNorm:
        if (!srgb) return VK_FORMAT_BC4_UNORM_BLOCK;
        break;
    case asset::TextureFormat::BC4SNorm:
        if (!srgb) return VK_FORMAT_BC4_SNORM_BLOCK;
        break;
    case asset::TextureFormat::BC5UNorm:
        if (!srgb) return VK_FORMAT_BC5_UNORM_BLOCK;
        break;
    case asset::TextureFormat::BC5SNorm:
        if (!srgb) return VK_FORMAT_BC5_SNORM_BLOCK;
        break;
    case asset::TextureFormat::BC6HUFloat:
        if (!srgb) return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        break;
    case asset::TextureFormat::BC6HSFloat:
        if (!srgb) return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        break;
    case asset::TextureFormat::BC7UNorm:
        return srgb
            ? VK_FORMAT_BC7_SRGB_BLOCK
            : VK_FORMAT_BC7_UNORM_BLOCK;
    case asset::TextureFormat::Undefined:
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
        return TextureFormatMapping{asset::TextureFormat::R8UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_R8_SRGB:
        return TextureFormatMapping{asset::TextureFormat::R8UNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_R8G8_UNORM:
        return TextureFormatMapping{asset::TextureFormat::RG8UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_R8G8_SRGB:
        return TextureFormatMapping{asset::TextureFormat::RG8UNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_R8G8B8A8_UNORM:
        return TextureFormatMapping{asset::TextureFormat::RGBA8UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_R8G8B8A8_SRGB:
        return TextureFormatMapping{asset::TextureFormat::RGBA8UNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return TextureFormatMapping{asset::TextureFormat::RGBA16Float, asset::TextureColorSpace::Linear};
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return TextureFormatMapping{asset::TextureFormat::RGBA32Float, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC1RGBUNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC1RGBUNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC1RGBAUNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC1RGBAUNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC2UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC2UNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC3UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC3UNorm, asset::TextureColorSpace::Srgb};
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC4UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC4SNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC5UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC5SNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC6HUFloat, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC6HSFloat, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC7UNorm, asset::TextureColorSpace::Linear};
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return TextureFormatMapping{asset::TextureFormat::BC7UNorm, asset::TextureColorSpace::Srgb};
    default:
        return std::nullopt;
    }
}

} // namespace rubia::rhi::vulkan
