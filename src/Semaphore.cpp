#include "Semaphore.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

Semaphore::Semaphore(VkDevice device)
{
    create(device);
}

Semaphore::~Semaphore()
{
    reset();
}

Semaphore::Semaphore(Semaphore&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      semaphore_(std::exchange(other.semaphore_, VK_NULL_HANDLE))
{
}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        semaphore_ = std::exchange(other.semaphore_, VK_NULL_HANDLE);
    }
    return *this;
}

void Semaphore::create(VkDevice device)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("cannot create Semaphore with an invalid device");
    }

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore newSemaphore = VK_NULL_HANDLE;
    if (vkCreateSemaphore(device, &createInfo, nullptr, &newSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create semaphore!");
    }

    reset();
    device_ = device;
    semaphore_ = newSemaphore;
}

void Semaphore::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && semaphore_ != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device_, semaphore_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    semaphore_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
