#pragma once

#include "Device.h"

#include <vector>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan command pool.
class CommandPool final
{
public:
    /// Creates an empty command-pool wrapper.
    CommandPool() = default;
    /// Creates a command pool for the selected queue family.
    CommandPool(
        const Device& device,
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0);
    /// Destroys the command pool and its command buffers.
    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    /// Transfers command-pool ownership from another wrapper.
    CommandPool(CommandPool&& other) noexcept;
    /// Replaces this command pool by taking ownership from another wrapper.
    CommandPool& operator=(CommandPool&& other) noexcept;

    /// Creates or replaces the command pool.
    void create(
        const Device& device,
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0);
    /// Resets all command buffers allocated from this pool.
    void resetCommands(VkCommandPoolResetFlags flags = 0) const;
    /// Destroys the command pool and clears its state.
    void reset() noexcept;

    /// Allocates one primary command buffer.
    [[nodiscard]] VkCommandBuffer allocatePrimary() const;
    /// Allocates the requested number of primary command buffers.
    [[nodiscard]] std::vector<VkCommandBuffer> allocatePrimary(uint32_t count) const;
    /// Frees one command buffer allocated from this pool.
    void free(VkCommandBuffer commandBuffer) const noexcept;
    /// Frees multiple command buffers allocated from this pool.
    void free(const std::vector<VkCommandBuffer>& commandBuffers) const noexcept;

    /// Returns the owned Vulkan command-pool handle.
    [[nodiscard]] VkCommandPool get() const noexcept { return commandPool_; }
    /// Returns whether a command pool is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept { return commandPool_ != VK_NULL_HANDLE; }

private:
    /// Logical device that owns the command pool.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan command-pool handle.
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
