#pragma once

#include "asset/TextureAsset.hpp"

#include <cstdint>
#include <optional>

namespace VkRenderer
{

/// KTX2 header format identifiers. KTX2 defines these numeric codes using the
/// Vulkan Format registry, but this serialization type deliberately exposes
/// neither VkFormat nor Vulkan headers to the asset pipeline.
enum class Ktx2FormatCode : uint32_t
{
    Undefined = 0,
    R8UNorm = 9,
    R8Srgb = 15,
    RG8UNorm = 16,
    RG8Srgb = 22,
    RGBA8UNorm = 37,
    RGBA8Srgb = 43,
    RGBA16Float = 97,
    RGBA32Float = 109,
    BC1RGBUNorm = 131,
    BC1RGBSrgb = 132,
    BC1RGBAUNorm = 133,
    BC1RGBASrgb = 134,
    BC2UNorm = 135,
    BC2Srgb = 136,
    BC3UNorm = 137,
    BC3Srgb = 138,
    BC4UNorm = 139,
    BC4SNorm = 140,
    BC5UNorm = 141,
    BC5SNorm = 142,
    BC6HUFloat = 143,
    BC6HSFloat = 144,
    BC7UNorm = 145,
    BC7Srgb = 146
};

struct Ktx2TextureFormatMapping
{
    TextureFormat format = TextureFormat::Undefined;
    TextureColorSpace colorSpace = TextureColorSpace::Linear;
};

[[nodiscard]] Ktx2FormatCode textureKtx2Format(
    TextureFormat format,
    TextureColorSpace colorSpace);

[[nodiscard]] std::optional<Ktx2TextureFormatMapping>
textureFormatFromKtx2(uint32_t formatCode) noexcept;

} // namespace VkRenderer
