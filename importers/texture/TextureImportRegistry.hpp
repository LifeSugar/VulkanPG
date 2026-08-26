#pragma once

#include "asset/AssetFwd.hpp"
#include "texture/TextureImportSettings.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace VkRenderer
{

/// Editor/import-domain provenance for one runtime texture. TextureAsset stays
/// source-independent and does not own these paths or cooking settings.
struct TextureImportRecord
{
    TextureAssetHandle texture;
    std::filesystem::path sourcePath;
    std::filesystem::path cookedPath;
    TextureImportSettings settings;
    uint64_t revision = 0;
    bool reimporting = false;
    std::string lastError;
};

struct TextureReimportRequest
{
    TextureAssetHandle texture;
    TextureImportSettings settings;
};

/// Owns texture source provenance without coupling AssetManager to importers.
class TextureImportRegistry final
{
public:
    void registerTexture(TextureImportRecord record);

    [[nodiscard]] const TextureImportRecord* find(
        TextureAssetHandle texture) const noexcept;
    [[nodiscard]] bool contains(TextureAssetHandle texture) const noexcept;

    void markStarted(TextureAssetHandle texture);
    void markSucceeded(
        TextureAssetHandle texture,
        TextureImportSettings settings);
    void markFailed(TextureAssetHandle texture, std::string error);
    void reset() noexcept;

    [[nodiscard]] std::size_t size() const noexcept
    {
        return records_.size();
    }

private:
    [[nodiscard]] static uint64_t key(TextureAssetHandle texture) noexcept;

    std::unordered_map<uint64_t, TextureImportRecord> records_;
};

} // namespace VkRenderer
