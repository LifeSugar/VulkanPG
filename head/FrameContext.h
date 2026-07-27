#pragma once

#include "CommandPool.h"
#include "Fence.h"
#include "Semaphore.h"

#include <vulkan/vulkan.h>

namespace VkRenderer
{

class Device;

// Resources selected by currentFrame. A FrameContext is reusable only after
// its in-flight fence signals; it deliberately owns no scene or swapchain data.
class FrameContext final
{
public:
    /// Creates an empty frame context.
    FrameContext() = default;
    /// Creates reusable frame resources on the supplied device.
    explicit FrameContext(const Device& device);
    /// Releases all owned frame resources.
    ~FrameContext() = default;

    FrameContext(const FrameContext&) = delete;
    FrameContext& operator=(const FrameContext&) = delete;
    /// Transfers all frame resources from another context.
    FrameContext(FrameContext&& other) noexcept;
    /// Replaces this context by taking resources from another context.
    FrameContext& operator=(FrameContext&& other) noexcept;

    /// Creates or replaces the resources for one frame slot.
    void create(const Device& device);
    /// Releases all resources owned by this frame slot.
    void reset() noexcept;

    /// Waits until the previous submission using this frame slot completes.
    void waitUntilReusable() const;
    /// Resets the command pool before recording the next frame.
    void resetCommands();
    /// Returns the in-flight fence to the unsignaled state.
    void resetFence();

    /// Returns the primary command buffer for this frame slot.
    [[nodiscard]] VkCommandBuffer commandBuffer() const noexcept
    {
        return commandBuffer_;
    }
    /// Returns the semaphore signaled when an image is acquired.
    [[nodiscard]] VkSemaphore imageAvailable() const noexcept
    {
        return imageAvailable_.get();
    }
    /// Returns the fence signaled when this frame's submission completes.
    [[nodiscard]] VkFence inFlightFence() const noexcept
    {
        return inFlight_.get();
    }

private:
    /// Command pool dedicated to this frame slot.
    CommandPool commandPool_;
    /// Primary command buffer allocated from the frame command pool.
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    /// Semaphore used to synchronize swapchain image acquisition.
    Semaphore imageAvailable_;
    /// Fence used to guard reuse of this frame slot.
    Fence inFlight_;
};

} // namespace VkRenderer
