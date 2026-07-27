#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

namespace VkRenderer
{

    /// Sentinel used when a renderer-owned data index is unavailable.
    inline constexpr uint32_t kInvalidRenderDataIndex =
        std::numeric_limits<uint32_t>::max();

    // A stable reference to a renderer-owned material. The generation prevents a
    // stale handle from silently resolving to a material that reused the same slot.
    struct MaterialHandle
    {
        /// Slot occupied by the material in renderer storage.
        uint32_t index = kInvalidRenderDataIndex;
        /// Slot generation used to reject stale handles.
        uint32_t generation = 0;

        /// Returns whether the handle references a material slot.
        [[nodiscard]] bool valid() const noexcept
        {
            return index != kInvalidRenderDataIndex;
        }

        /// Returns whether the handle is valid.
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return valid();
        }
    };

    /// Compares both the slot and generation of two material handles.
    [[nodiscard]] inline bool operator==(
        const MaterialHandle &lhs,
        const MaterialHandle &rhs) noexcept
    {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }

    /// Returns whether two material handles differ.
    [[nodiscard]] inline bool operator!=(
        const MaterialHandle &lhs,
        const MaterialHandle &rhs) noexcept
    {
        return !(lhs == rhs);
    }

    // One snapshot per CPU/GPU frame slot. Keep the structure 16-byte aligned so
    // it can be copied directly into a uniform-buffer allocation.
    struct alignas(16) FrameGpuData
    {
        /// Elapsed application time in seconds.
        float time = 0.0f;
        /// Time elapsed since the previous frame in seconds.
        float deltaTime = 0.0f;
        /// Monotonic frame number.
        uint32_t frameIndex = 0;
        /// Padding required by the GPU data layout.
        uint32_t padding = 0;
    };

    // Camera remains the CPU-side source of truth. This is only the compact data
    // snapshot consumed by shaders for a camera render flow.
    struct alignas(16) CameraGpuData
    {
        /// Combined view and projection transform.
        glm::mat4 viewProjection{1.0f};
        /// Camera position in world space.
        glm::vec4 worldPosition{0.0f, 0.0f, 0.0f, 1.0f};
    };

    // Per-object data is uploaded as a contiguous array for the current frame.
    // DrawPushConstants::objectIndex selects an entry from that array.
    struct alignas(16) ObjectGpuData
    {
        /// Object-to-world transform.
        glm::mat4 world{1.0f};
        /// Transform used for surface normals.
        glm::mat4 normalMatrix{1.0f};
    };

    // CPU-side render-queue entry. It references renderer resources and GPU data;
    // it is not uploaded to a shader as-is.
    struct DrawItem
    {
        /// Renderer mesh slot used by this draw.
        uint32_t meshIndex = kInvalidRenderDataIndex;
        /// Submesh slot within the selected mesh.
        uint32_t submeshIndex = kInvalidRenderDataIndex;
        /// Object-data slot selected for this draw.
        uint32_t objectIndex = kInvalidRenderDataIndex;
        /// Material referenced by this draw.
        MaterialHandle material;
    };

    // Small, frequently changing indices used to select the camera and object
    // records while recording a draw.
    struct DrawPushConstants
    {
        /// Camera-data slot selected by the draw.
        uint32_t cameraIndex = 0;
        /// Object-data slot selected by the draw.
        uint32_t objectIndex = 0;
    };

    static_assert(sizeof(FrameGpuData) == 16);
    static_assert(alignof(FrameGpuData) == 16);
    static_assert(sizeof(CameraGpuData) == 80);
    static_assert(alignof(CameraGpuData) == 16);
    static_assert(offsetof(CameraGpuData, worldPosition) == 64);
    static_assert(sizeof(ObjectGpuData) == 128);
    static_assert(alignof(ObjectGpuData) == 16);
    static_assert(offsetof(ObjectGpuData, normalMatrix) == 64);
    static_assert(sizeof(DrawPushConstants) == 8);

} // namespace VkRenderer
