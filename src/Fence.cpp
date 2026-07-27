#include "Fence.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

Fence::Fence(VkDevice device, VkFenceCreateFlags flags)
{
    create(device, flags);
}

Fence::~Fence()
{
    reset();
}

Fence::Fence(Fence&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      fence_(std::exchange(other.fence_, VK_NULL_HANDLE))
{
}

Fence& Fence::operator=(Fence&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        fence_ = std::exchange(other.fence_, VK_NULL_HANDLE);
    }
    return *this;
}

void Fence::create(VkDevice device, VkFenceCreateFlags flags)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("cannot create Fence with an invalid device");
    }

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = flags;

    VkFence newFence = VK_NULL_HANDLE;
    if (vkCreateFence(device, &createInfo, nullptr, &newFence) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create fence!");
    }

    reset();
    device_ = device;
    fence_ = newFence;
}

void Fence::wait(uint64_t timeout) const
{
    if (device_ == VK_NULL_HANDLE || fence_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot wait for an empty Fence");
    }
    if (vkWaitForFences(device_, 1, &fence_, VK_TRUE, timeout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to wait for fence!");
    }
}

void Fence::resetSignal() const
{
    if (device_ == VK_NULL_HANDLE || fence_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot reset an empty Fence");
    }
    if (vkResetFences(device_, 1, &fence_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to reset fence!");
    }
}

void Fence::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && fence_ != VK_NULL_HANDLE)
    {
        vkDestroyFence(device_, fence_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    fence_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
