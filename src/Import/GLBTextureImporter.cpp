#include "Import/GLBTextureImporter.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

std::vector<TextureAssetHandle> GLBTextureImporter::import(
    const std::vector<GLBTexture>& source,
    const CreateInfo& createInfo) const
{
    if (createInfo.assets == nullptr)
    {
        throw std::invalid_argument(
            "GLBTextureImporter requires an AssetManager");
    }
    if (createInfo.fallbackTexture &&
        !createInfo.assets->contains(createInfo.fallbackTexture))
    {
        throw std::invalid_argument(
            "GLBTextureImporter fallback texture is not owned by its AssetManager");
    }

    std::vector<TextureAssetHandle> result;
    result.reserve(source.size());
    for (const GLBTexture& texture : source)
    {
        TextureAsset::CreateInfo textureInfo{};
        if (texture.storage == GLBTextureStorage::Rgba8Pixels)
        {
            if (texture.width == 0 || texture.height == 0 ||
                static_cast<std::size_t>(texture.width) >
                    std::numeric_limits<std::size_t>::max() /
                        texture.height)
            {
                throw std::invalid_argument(
                    "GLB RGBA8 texture dimensions are invalid: " +
                    texture.name);
            }
            const std::size_t pixelCount =
                static_cast<std::size_t>(texture.width) * texture.height;
            if (pixelCount >
                std::numeric_limits<std::size_t>::max() / 4 ||
                texture.data.size() != pixelCount * 4)
            {
                throw std::invalid_argument(
                    "GLB RGBA8 texture payload size does not match its dimensions: " +
                    texture.name);
            }

            textureInfo.name = texture.name;
            textureInfo.width = texture.width;
            textureInfo.height = texture.height;
            textureInfo.format = TextureFormat::RGBA8UNorm;
            textureInfo.colorSpace = createInfo.colorSpace;
            textureInfo.sampler = createInfo.sampler;
            textureInfo.pixels.resize(texture.data.size());
            std::memcpy(
                textureInfo.pixels.data(),
                texture.data.data(),
                texture.data.size());
        }
        else if (createInfo.decoder)
        {
            textureInfo = createInfo.decoder(
                texture,
                createInfo.baseDirectory);
            if (textureInfo.name.empty())
            {
                textureInfo.name = texture.name;
            }
            textureInfo.colorSpace = createInfo.colorSpace;
            textureInfo.sampler = createInfo.sampler;
        }
        else if (createInfo.fallbackTexture)
        {
            result.push_back(createInfo.fallbackTexture);
            continue;
        }
        else
        {
            throw std::runtime_error(
                "GLB texture requires an external or encoded-image decoder: " +
                texture.uri);
        }

        result.push_back(
            createInfo.assets->createTexture(std::move(textureInfo)));
    }
    return result;
}

} // namespace VkRenderer
