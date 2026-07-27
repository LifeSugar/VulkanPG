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
    FrameContext() = default;
    explicit FrameContext(const Device& device);
    ~FrameContext() = default;

    FrameContext(const FrameContext&) = delete;
    FrameContext& operator=(const FrameContext&) = delete;
    FrameContext(FrameContext&& other) noexcept;
    FrameContext& operator=(FrameContext&& other) noexcept;

    void create(const Device& device);
    void reset() noexcept;

    void waitUntilReusable() const;
    void resetCommands();
    void resetFence();

    [[nodiscard]] VkCommandBuffer commandBuffer() const noexcept
    {
        return commandBuffer_;
    }
    [[nodiscard]] VkSemaphore imageAvailable() const noexcept
    {
        return imageAvailable_.get();
    }
    [[nodiscard]] VkFence inFlightFence() const noexcept
    {
        return inFlight_.get();
    }

private:
    CommandPool commandPool_;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    Semaphore imageAvailable_;
    Fence inFlight_;
};

} // namespace VkRenderer
