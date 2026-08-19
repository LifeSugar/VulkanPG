#pragma once

#include "Asset/AssetFwd.h"

#include <cstdint>

namespace VkRenderer
{

/// Stable, backend-independent identity of one material instance.
///
/// This intentionally mirrors the asset handle rather than a VkDescriptorSet:
/// GPU handles are neither deterministic nor valid outside one cache lifetime.
struct MaterialKey
{
    uint32_t assetIndex = kInvalidAssetIndex;
    uint32_t assetGeneration = 0;

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return assetIndex != kInvalidAssetIndex;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return valid();
    }
};

[[nodiscard]] constexpr MaterialKey makeMaterialKey(
    MaterialAssetHandle material) noexcept
{
    return {material.index, material.generation};
}

[[nodiscard]] constexpr bool operator==(
    MaterialKey left,
    MaterialKey right) noexcept
{
    return left.assetIndex == right.assetIndex &&
        left.assetGeneration == right.assetGeneration;
}

[[nodiscard]] constexpr bool operator!=(
    MaterialKey left,
    MaterialKey right) noexcept
{
    return !(left == right);
}

/// Provides a total, reproducible order independent of GPU allocation order.
struct MaterialKeyLess
{
    [[nodiscard]] constexpr bool operator()(
        MaterialKey left,
        MaterialKey right) const noexcept
    {
        if (left.assetIndex != right.assetIndex)
        {
            return left.assetIndex < right.assetIndex;
        }
        return left.assetGeneration < right.assetGeneration;
    }
};

static_assert(
    makeMaterialKey(MaterialAssetHandle{7, 3}) == MaterialKey{7, 3});
static_assert(MaterialKeyLess{}(MaterialKey{1, 9}, MaterialKey{2, 1}));
static_assert(MaterialKeyLess{}(MaterialKey{2, 1}, MaterialKey{2, 2}));

} // namespace VkRenderer
