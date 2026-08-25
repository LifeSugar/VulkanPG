#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace VkRenderer
{

/// Stable, serializable identity for a logical project asset.
///
/// Unlike AssetHandle<T>, an AssetId may be written to project metadata and
/// remains meaningful across process launches and AssetManager rebuilds.
struct AssetId
{
    uint64_t high = 0;
    uint64_t low = 0;

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return high != 0 || low != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return valid();
    }

    /// Creates a random RFC 4122 variant, version 4 identifier.
    [[nodiscard]] static AssetId generate();

    /// Parses the canonical 8-4-4-4-12 hexadecimal representation.
    /// Uppercase input is accepted; malformed input returns std::nullopt.
    [[nodiscard]] static std::optional<AssetId> parse(
        std::string_view text) noexcept;

    /// Returns the canonical lowercase 8-4-4-4-12 representation.
    [[nodiscard]] std::string toString() const;
};

[[nodiscard]] constexpr bool operator==(
    AssetId lhs,
    AssetId rhs) noexcept
{
    return lhs.high == rhs.high && lhs.low == rhs.low;
}

[[nodiscard]] constexpr bool operator!=(
    AssetId lhs,
    AssetId rhs) noexcept
{
    return !(lhs == rhs);
}

[[nodiscard]] constexpr bool operator<(
    AssetId lhs,
    AssetId rhs) noexcept
{
    return lhs.high < rhs.high ||
        (lhs.high == rhs.high && lhs.low < rhs.low);
}

struct AssetIdHash
{
    [[nodiscard]] std::size_t operator()(AssetId id) const noexcept;
};

} // namespace VkRenderer

namespace std
{

template <>
struct hash<VkRenderer::AssetId>
{
    [[nodiscard]] size_t operator()(
        VkRenderer::AssetId id) const noexcept
    {
        return VkRenderer::AssetIdHash{}(id);
    }
};

} // namespace std
