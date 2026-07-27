#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

class Semaphore final
{
public:
    Semaphore() = default;
    explicit Semaphore(VkDevice device);
    ~Semaphore();

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&& other) noexcept;
    Semaphore& operator=(Semaphore&& other) noexcept;

    void create(VkDevice device);
    void reset() noexcept;

    [[nodiscard]] VkSemaphore get() const noexcept { return semaphore_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return semaphore_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
