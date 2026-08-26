#include "texture/TextureImportRegistry.hpp"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

uint64_t TextureImportRegistry::key(TextureAssetHandle texture) noexcept
{
    return (static_cast<uint64_t>(texture.generation) << 32u) |
        static_cast<uint64_t>(texture.index);
}

void TextureImportRegistry::registerTexture(TextureImportRecord record)
{
    if (!record.texture || record.sourcePath.empty() ||
        record.cookedPath.empty())
    {
        throw std::invalid_argument(
            "texture import record requires a texture and source/cooked paths");
    }
    records_.insert_or_assign(key(record.texture), std::move(record));
}

const TextureImportRecord* TextureImportRegistry::find(
    TextureAssetHandle texture) const noexcept
{
    if (!texture)
    {
        return nullptr;
    }
    const auto iterator = records_.find(key(texture));
    return iterator == records_.end() ? nullptr : &iterator->second;
}

bool TextureImportRegistry::contains(TextureAssetHandle texture) const noexcept
{
    return find(texture) != nullptr;
}

void TextureImportRegistry::markStarted(TextureAssetHandle texture)
{
    auto iterator = records_.find(key(texture));
    if (iterator == records_.end())
    {
        throw std::out_of_range("texture import record is absent");
    }
    if (iterator->second.reimporting)
    {
        throw std::logic_error("texture reimport is already in progress");
    }
    iterator->second.reimporting = true;
    iterator->second.lastError.clear();
}

void TextureImportRegistry::markSucceeded(
    TextureAssetHandle texture,
    TextureImportSettings settings)
{
    auto iterator = records_.find(key(texture));
    if (iterator == records_.end())
    {
        throw std::out_of_range("texture import record is absent");
    }
    iterator->second.settings = std::move(settings);
    ++iterator->second.revision;
    iterator->second.reimporting = false;
    iterator->second.lastError.clear();
}

void TextureImportRegistry::markFailed(
    TextureAssetHandle texture,
    std::string error)
{
    auto iterator = records_.find(key(texture));
    if (iterator == records_.end())
    {
        return;
    }
    iterator->second.reimporting = false;
    iterator->second.lastError = std::move(error);
}

void TextureImportRegistry::reset() noexcept
{
    records_.clear();
}

} // namespace VkRenderer
