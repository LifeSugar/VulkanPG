#pragma once

#include "asset/TextureAsset.hpp"

#include <vulkan/vulkan.h>

#include <optional>

namespace VkRenderer
{

struct TextureFormatMapping
{
    TextureFormat format = TextureFormat::Undefined;
    TextureColorSpace colorSpace = TextureColorSpace::Linear;
};

/// Converts an engine texture format and color space to Vulkan.
[[nodiscard]] VkFormat textureVkFormat(
    TextureFormat format,
    TextureColorSpace colorSpace);

/// Converts a Vulkan format supported by TextureAsset back to engine metadata.
[[nodiscard]] std::optional<TextureFormatMapping>
textureFormatFromVk(VkFormat format) noexcept;

} // namespace VkRenderer
