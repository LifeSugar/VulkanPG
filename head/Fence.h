#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace VkRenderer
{

class Fence final
{
public:
    Fence() = default;
    Fence(VkDevice device, VkFenceCreateFlags flags = 0);
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&& other) noexcept;
    Fence& operator=(Fence&& other) noexcept;

    void create(VkDevice device, VkFenceCreateFlags flags = 0);
    void wait(uint64_t timeout = UINT64_MAX) const;
    void resetSignal() const;
    void reset() noexcept;

    [[nodiscard]] VkFence get() const noexcept { return fence_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return fence_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
