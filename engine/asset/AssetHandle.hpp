#pragma once

#include <cstdint>
#include <limits>

namespace VkRenderer
{

inline constexpr uint32_t kInvalidAssetIndex =
    std::numeric_limits<uint32_t>::max();

/// Type-safe, generation-checked reference to an AssetManager entry.
template <typename Asset>
struct AssetHandle
{
    uint32_t index = kInvalidAssetIndex;
    uint32_t generation = 0;

    [[nodiscard]] bool valid() const noexcept
    {
        return index != kInvalidAssetIndex;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return valid();
    }
};

template <typename Asset>
[[nodiscard]] bool operator==(
    AssetHandle<Asset> lhs,
    AssetHandle<Asset> rhs) noexcept
{
    return lhs.index == rhs.index && lhs.generation == rhs.generation;
}

template <typename Asset>
[[nodiscard]] bool operator!=(
    AssetHandle<Asset> lhs,
    AssetHandle<Asset> rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace VkRenderer
