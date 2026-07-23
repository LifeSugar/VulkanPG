#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

namespace VkRenderer
{

    inline constexpr uint32_t kInvalidRenderDataIndex =
        std::numeric_limits<uint32_t>::max();

    // A stable reference to a renderer-owned material. The generation prevents a
    // stale handle from silently resolving to a material that reused the same slot.
    struct MaterialHandle
    {
        uint32_t index = kInvalidRenderDataIndex;
        uint32_t generation = 0;

        [[nodiscard]] bool valid() const noexcept
        {
            return index != kInvalidRenderDataIndex;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return valid();
        }
    };

    [[nodiscard]] inline bool operator==(
        const MaterialHandle &lhs,
        const MaterialHandle &rhs) noexcept
    {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }

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
        float time = 0.0f;
        float deltaTime = 0.0f;
        uint32_t frameIndex = 0;
        uint32_t padding = 0;
    };

    // Camera remains the CPU-side source of truth. This is only the compact data
    // snapshot consumed by shaders for a camera render flow.
    struct alignas(16) CameraGpuData
    {
        glm::mat4 viewProjection{1.0f};
        glm::vec4 worldPosition{0.0f, 0.0f, 0.0f, 1.0f};
    };

    // Per-object data is uploaded as a contiguous array for the current frame.
    // DrawPushConstants::objectIndex selects an entry from that array.
    struct alignas(16) ObjectGpuData
    {
        glm::mat4 world{1.0f};
        glm::mat4 normalMatrix{1.0f};
    };

    // CPU-side render-queue entry. It references renderer resources and GPU data;
    // it is not uploaded to a shader as-is.
    struct DrawItem
    {
        uint32_t meshIndex = kInvalidRenderDataIndex;
        uint32_t submeshIndex = kInvalidRenderDataIndex;
        uint32_t objectIndex = kInvalidRenderDataIndex;
        MaterialHandle material;
    };

    // Small, frequently changing indices used to select the camera and object
    // records while recording a draw.
    struct DrawPushConstants
    {
        uint32_t cameraIndex = 0;
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
