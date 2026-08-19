#pragma once

#include "Asset/MaterialState.h"

#include <cstdint>

namespace VkRenderer
{

/// Coarse draw order shared by material classification and list sorting.
enum class RenderQueue : uint16_t
{
    /// Regular depth-writing geometry.
    Opaque = 2000,
    /// Discard-based geometry rendered after regular opaque draws.
    AlphaClip = 2450,
    /// Alpha-blended geometry rendered after all depth-writing draws.
    Transparent = 3000
};

[[nodiscard]] constexpr uint16_t renderQueueValue(
    RenderQueue queue) noexcept
{
    return static_cast<uint16_t>(queue);
}

/// Maps backend-independent material state to its canonical draw queue.
[[nodiscard]] constexpr RenderQueue renderQueueFor(
    const MaterialRenderState& state) noexcept
{
    if (state.transparent())
    {
        return RenderQueue::Transparent;
    }
    return state.alphaClipEnabled
        ? RenderQueue::AlphaClip
        : RenderQueue::Opaque;
}

[[nodiscard]] constexpr bool isTransparentQueue(
    RenderQueue queue) noexcept
{
    return renderQueueValue(queue) >=
        renderQueueValue(RenderQueue::Transparent);
}

static_assert(
    renderQueueValue(RenderQueue::Opaque) <
    renderQueueValue(RenderQueue::AlphaClip));
static_assert(
    renderQueueValue(RenderQueue::AlphaClip) <
    renderQueueValue(RenderQueue::Transparent));
static_assert(
    renderQueueFor(makeOpaqueMaterialState()) == RenderQueue::Opaque);
static_assert(
    renderQueueFor(makeTransparentMaterialState()) ==
    RenderQueue::Transparent);

} // namespace VkRenderer
