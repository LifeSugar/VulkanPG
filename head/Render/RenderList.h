#pragma once

#include "Render/RenderItem.h"
#include "RenderData.h"

#include <cstddef>
#include <vector>

namespace VkRenderer
{

/// View-specific, ordered draw lists produced from render candidates.
struct RenderList
{
    /// Packed transforms addressed by RenderItem::objectIndex.
    std::vector<ObjectGpuData> objectData;
    /// Depth-writing draws; alpha-clipped materials remain in this list.
    std::vector<RenderItem> opaque;
    /// Alpha-blended draws, eventually ordered from back to front.
    std::vector<RenderItem> transparent;

    [[nodiscard]] std::size_t size() const noexcept
    {
        return opaque.size() + transparent.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return opaque.empty() && transparent.empty();
    }

    void clear() noexcept
    {
        objectData.clear();
        opaque.clear();
        transparent.clear();
    }
};

} // namespace VkRenderer
