#pragma once

#include "Asset/TextureAsset.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace VkRenderer
{

/// Decodes common encoded image formats into RGBA8 CPU pixels through WIC.
class WicImageDecoder final
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
