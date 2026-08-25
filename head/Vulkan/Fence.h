#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan fence.
class Fence final
{
public:
    /// Creates an empty fence wrapper.
    Fence() = default;
    /// Creates a Vulkan fence with the requested flags.
    Fence(VkDevice device, VkFenceCreateFlags flags = 0);
    /// Destroys the owned fence.
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    /// Transfers fence ownership from another wrapper.
    Fence(Fence&& other) noexcept;
    /// Replaces this fence by taking ownership from another wrapper.
    Fence& operator=(Fence&& other) noexcept;

    /// Creates or replaces the owned fence.
    void create(VkDevice device, VkFenceCreateFlags flags = 0);
    /// Waits for the fence to signal or the timeout to expire.
    void wait(uint64_t timeout = UINT64_MAX) const;
    /// Returns the fence to the unsignaled state.
    void resetSignal() const;
    /// Destroys the owned fence and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan fence handle.
    [[nodiscard]] VkFence get() const noexcept { return fence_; }
    /// Returns whether a fence is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return fence_ != VK_NULL_HANDLE;
    }

private:
    /// Logical device that owns the fence.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan fence handle.
    VkFence fence_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
