#pragma once

#include <cstdint>

namespace VkRenderer
{

/// Visibility tests enabled for one Camera/RenderView.
enum class CullingFlags : uint8_t
{
    None = 0,
    LayerMask = 1u << 0,
    Frustum = 1u << 1,
    All = (1u << 0) | (1u << 1)
};

[[nodiscard]] constexpr CullingFlags operator|(
    CullingFlags left,
    CullingFlags right) noexcept
{
    return static_cast<CullingFlags>(
        static_cast<uint8_t>(left) |
        static_cast<uint8_t>(right));
}

[[nodiscard]] constexpr CullingFlags operator&(
    CullingFlags left,
    CullingFlags right) noexcept
{
    return static_cast<CullingFlags>(
        static_cast<uint8_t>(left) &
        static_cast<uint8_t>(right));
}

[[nodiscard]] constexpr bool hasCullingFlag(
    CullingFlags flags,
    CullingFlags flag) noexcept
{
    return (flags & flag) == flag;
}

/// Per-object control over geometric bounds culling.
enum class BoundsCullingMode : uint8_t
{
    Normal,
    Disabled
};

static_assert(hasCullingFlag(CullingFlags::All, CullingFlags::LayerMask));
static_assert(hasCullingFlag(CullingFlags::All, CullingFlags::Frustum));

} // namespace VkRenderer
