#pragma once

#include "Asset/TextureAsset.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace VkRenderer
{

/// Decodes common encoded image formats into a tightly packed RGBA8 payload.
/// This decoder has no platform-specific runtime dependencies.
class StbImageDecoder final
{
public:
    [[nodiscard]] TextureAsset::CreateInfo decodeMemory(
        const std::vector<uint8_t>& encodedBytes,
        const std::string& name) const;

    [[nodiscard]] TextureAsset::CreateInfo decodeFile(
        const std::filesystem::path& path,
        const std::string& name = {}) const;
};

} // namespace VkRenderer
