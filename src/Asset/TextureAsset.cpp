#include "Asset/TextureAsset.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

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
    if (createInfo.pixels.empty())
    {
        throw std::invalid_argument("texture pixel payload must not be empty");
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
            createInfo.pixels.size()
        });
    }

    for (const TextureMipLevel& mip : createInfo.mipLevels)
    {
        if (mip.width == 0 || mip.height == 0 || mip.byteSize == 0 ||
            mip.byteOffset > createInfo.pixels.size() ||
            mip.byteSize > createInfo.pixels.size() - mip.byteOffset)
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
    pixels_ = std::move(createInfo.pixels);
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
    pixels_.clear();
    mipLevels_.clear();
}

} // namespace VkRenderer
