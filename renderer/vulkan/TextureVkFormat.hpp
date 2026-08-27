#pragma once

#include "asset/TextureAsset.hpp"

#include <vulkan/vulkan.h>

#include <optional>

namespace rubia::rhi::vulkan
{

struct TextureFormatMapping
{
    asset::TextureFormat format = asset::TextureFormat::Undefined;
    asset::TextureColorSpace colorSpace = asset::TextureColorSpace::Linear;
};

/// Converts an engine texture format and color space to Vulkan.
[[nodiscard]] VkFormat textureVkFormat(
    asset::TextureFormat format,
    asset::TextureColorSpace colorSpace);

/// Converts a Vulkan format supported by TextureAsset back to engine metadata.
[[nodiscard]] std::optional<TextureFormatMapping>
textureFormatFromVk(VkFormat format) noexcept;

} // namespace rubia::rhi::vulkan
