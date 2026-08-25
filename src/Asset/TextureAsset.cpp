#include "Asset/TextureAsset.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

TextureFormatInfo textureFormatInfo(TextureFormat format) noexcept
{
    switch (format)
    {
    case TextureFormat::R8UNorm:
        return {1, 1, 1, false, true};
    case TextureFormat::RG8UNorm:
        return {1, 1, 2, false, true};
    case TextureFormat::RGBA8UNorm:
        return {1, 1, 4, false, true};
    case TextureFormat::RGBA16Float:
        return {1, 1, 8, false, false};
    case TextureFormat::RGBA32Float:
        return {1, 1, 16, false, false};
    case TextureFormat::BC1RGBUNorm:
    case TextureFormat::BC1RGBAUNorm:
        return {4, 4, 8, true, true};
    case TextureFormat::BC2UNorm:
    case TextureFormat::BC3UNorm:
        return {4, 4, 16, true, true};
    case TextureFormat::BC4UNorm:
    case TextureFormat::BC4SNorm:
        return {4, 4, 8, true, false};
    case TextureFormat::BC5UNorm:
    case TextureFormat::BC5SNorm:
    case TextureFormat::BC6HUFloat:
    case TextureFormat::BC6HSFloat:
        return {4, 4, 16, true, false};
    case TextureFormat::BC7UNorm:
        return {4, 4, 16, true, true};
    case TextureFormat::Undefined:
        break;
    }
    return {};
}

std::size_t textureMipByteSize(
    TextureFormat format,
    uint32_t width,
    uint32_t height)
{
    const TextureFormatInfo info = textureFormatInfo(format);
    if (info.bytesPerBlock == 0 || width == 0 || height == 0)
    {
        throw std::invalid_argument(
            "texture mip byte size requires a format and dimensions");
    }

    const std::size_t blockCountX =
        1 + (static_cast<std::size_t>(width) - 1) / info.blockWidth;
    const std::size_t blockCountY =
        1 + (static_cast<std::size_t>(height) - 1) / info.blockHeight;
    if (blockCountX >
            std::numeric_limits<std::size_t>::max() / blockCountY ||
        blockCountX * blockCountY >
            std::numeric_limits<std::size_t>::max() / info.bytesPerBlock)
    {
        throw std::overflow_error("texture mip byte size overflows size_t");
    }
    return blockCountX * blockCountY * info.bytesPerBlock;
}

TextureAsset::TextureAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void TextureAsset::create(CreateInfo createInfo)
{
    if (createInfo.width == 0 || createInfo.height == 0)
    {
        throw std::invalid_argument("texture dimensions must be non-zero");
    }
    if (createInfo.format == TextureFormat::Undefined)
    {
        throw std::invalid_argument("texture format must be defined");
    }
    const TextureFormatInfo formatInfo = textureFormatInfo(createInfo.format);
    if (createInfo.colorSpace == TextureColorSpace::Srgb &&
        !formatInfo.supportsSrgb)
    {
        throw std::invalid_argument(
            "texture format does not have an sRGB representation");
    }
    if (createInfo.payload.empty())
    {
        throw std::invalid_argument("texture payload must not be empty");
    }
    if (!std::isfinite(createInfo.sampler.maxAnisotropy) ||
        createInfo.sampler.maxAnisotropy < 1.0f)
    {
        throw std::invalid_argument("texture anisotropy must be finite and at least one");
    }

    if (createInfo.mipLevels.empty())
    {
        createInfo.mipLevels.push_back({
            createInfo.width,
            createInfo.height,
            0,
            createInfo.payload.size()
        });
    }

    for (const TextureMipLevel& mip : createInfo.mipLevels)
    {
        if (mip.width == 0 || mip.height == 0 ||
            mip.byteSize != textureMipByteSize(
                createInfo.format,
                mip.width,
                mip.height) ||
            mip.byteOffset > createInfo.payload.size() ||
            mip.byteSize > createInfo.payload.size() - mip.byteOffset)
        {
            throw std::invalid_argument("texture mip range is invalid");
        }
    }

    name_ = std::move(createInfo.name);
    width_ = createInfo.width;
    height_ = createInfo.height;
    format_ = createInfo.format;
    colorSpace_ = createInfo.colorSpace;
    sampler_ = createInfo.sampler;
    payload_ = std::move(createInfo.payload);
    mipLevels_ = std::move(createInfo.mipLevels);
}

void TextureAsset::reset() noexcept
{
    name_.clear();
    width_ = 0;
    height_ = 0;
    format_ = TextureFormat::Undefined;
    colorSpace_ = TextureColorSpace::Linear;
    sampler_ = {};
    payload_.clear();
    mipLevels_.clear();
}

} // namespace VkRenderer
