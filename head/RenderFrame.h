#pragma once

#include "Render/RenderList.h"
#include "Render/RenderView.h"

namespace VkRenderer
{

/// Complete renderer input for one frame.
struct RenderFrame
{
    /// Camera snapshot shared by all lists in the frame.
    RenderView view;
    /// View-specific opaque and transparent draw lists.
    RenderList renderList;
};

} // namespace VkRenderer
