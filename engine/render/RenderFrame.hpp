#pragma once

#include "render/RenderList.hpp"
#include "render/RenderView.hpp"

namespace rubia::render
{

/// Complete renderer input for one frame.
struct RenderFrame
{
    /// Camera snapshot shared by all lists in the frame.
    RenderView view;
    /// View-specific opaque and transparent draw lists.
    RenderList renderList;
};

} // namespace rubia::render
