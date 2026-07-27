#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan semaphore.
class Semaphore final
{
public:
    /// Creates an empty semaphore wrapper.
    Semaphore() = default;
    /// Creates a Vulkan semaphore on the supplied device.
    explicit Semaphore(VkDevice device);
    /// Destroys the owned semaphore.
    ~Semaphore();

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    /// Transfers semaphore ownership from another wrapper.
    Semaphore(Semaphore&& other) noexcept;
    /// Replaces this semaphore by taking ownership from another wrapper.
    Semaphore& operator=(Semaphore&& other) noexcept;

    /// Creates or replaces the owned semaphore.
    void create(VkDevice device);
    /// Destroys the owned semaphore and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan semaphore handle.
    [[nodiscard]] VkSemaphore get() const noexcept { return semaphore_; }
    /// Returns whether a semaphore is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return semaphore_ != VK_NULL_HANDLE;
    }

private:
    /// Logical device that owns the semaphore.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan semaphore handle.
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
