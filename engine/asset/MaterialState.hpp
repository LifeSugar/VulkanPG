#pragma once

#include <cstdint>

namespace VkRenderer
{

/// High-level surface classification used to select a render list.
enum class MaterialSurfaceType : uint8_t
{
    Opaque,
    Transparent
};

/// Backend-independent depth comparison operation.
enum class DepthCompare : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/// Depth state requested by a material pipeline variant.
struct MaterialDepthState
{
    bool testEnabled = true;
    bool writeEnabled = true;
    DepthCompare compare = DepthCompare::LessEqual;
};

/// Backend-independent render state compiled with one material instance.
struct MaterialRenderState
{
    MaterialSurfaceType surfaceType = MaterialSurfaceType::Opaque;
    bool alphaClipEnabled = false;
    float alphaClipThreshold = 0.5f;
    bool doubleSided = false;
    MaterialDepthState depth;

    [[nodiscard]] constexpr bool opaque() const noexcept
    {
        return surfaceType == MaterialSurfaceType::Opaque;
    }

    [[nodiscard]] constexpr bool transparent() const noexcept
    {
        return surfaceType == MaterialSurfaceType::Transparent;
    }
};

/// Canonical state for regular and alpha-clipped opaque materials.
[[nodiscard]] constexpr MaterialRenderState makeOpaqueMaterialState() noexcept
{
    return {};
}

/// Canonical alpha-blended state: depth-tested without writing depth.
[[nodiscard]] constexpr MaterialRenderState
makeTransparentMaterialState() noexcept
{
    MaterialRenderState state{};
    state.surfaceType = MaterialSurfaceType::Transparent;
    state.depth.writeEnabled = false;
    return state;
}

static_assert(makeOpaqueMaterialState().depth.testEnabled);
static_assert(makeOpaqueMaterialState().depth.writeEnabled);
static_assert(makeTransparentMaterialState().depth.testEnabled);
static_assert(!makeTransparentMaterialState().depth.writeEnabled);

} // namespace VkRenderer
