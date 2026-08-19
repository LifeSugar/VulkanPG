#pragma once

#include "Render/CullingSettings.h"
#include "Render/RenderLayer.h"
#include "RenderData.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace VkRenderer
{

/// Stable identity of the persistent source that produced a RenderView.
struct RenderViewId
{
    uint64_t value = 0;

    /// Allocates a process-unique identity for a persistent view source.
    [[nodiscard]] static RenderViewId generate();
    [[nodiscard]] bool valid() const noexcept { return value != 0; }
    [[nodiscard]] explicit operator bool() const noexcept { return valid(); }
};

[[nodiscard]] constexpr bool operator==(
    RenderViewId lhs,
    RenderViewId rhs) noexcept
{
    return lhs.value == rhs.value;
}

[[nodiscard]] constexpr bool operator!=(
    RenderViewId lhs,
    RenderViewId rhs) noexcept
{
    return !(lhs == rhs);
}

/// Immutable per-frame snapshot describing where and how one view renders.
struct RenderView
{
    /// Identifies the Camera or other persistent view source.
    RenderViewId id;
    /// Changes whenever the shader-visible view payload changes.
    uint64_t gpuDataRevision = 0;

    /// CPU-side view data used by culling and future depth sorting.
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 viewProjectionMatrix{1.0f};
    glm::vec3 worldPosition{0.0f};

    /// Compact snapshot uploaded to the camera uniform buffer.
    CameraGpuData gpuData{};

    /// Scene layers accepted by this view.
    LayerMask cullingMask = LayerMask::all();
    /// Visibility tests enabled for this view.
    CullingFlags cullingFlags = CullingFlags::All;
};

} // namespace VkRenderer
