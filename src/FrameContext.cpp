#include "FrameContext.h"

#include "Device.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

FrameContext::FrameContext(const Device& device)
{
    create(device);
}

FrameContext::FrameContext(FrameContext&& other) noexcept
    : commandPool_(std::move(other.commandPool_)),
      commandBuffer_(std::exchange(other.commandBuffer_, VK_NULL_HANDLE)),
      imageAvailable_(std::move(other.imageAvailable_)),
      inFlight_(std::move(other.inFlight_))
{
}

FrameContext& FrameContext::operator=(FrameContext&& other) noexcept
{
    if (this != &other)
    {
        reset();
        commandPool_ = std::move(other.commandPool_);
        commandBuffer_ = std::exchange(other.commandBuffer_, VK_NULL_HANDLE);
        imageAvailable_ = std::move(other.imageAvailable_);
        inFlight_ = std::move(other.inFlight_);
    }
    return *this;
}

void FrameContext::create(const Device& device)
{
    if (!device)
    {
        throw std::invalid_argument("cannot create FrameContext with an invalid device");
    }

    CommandPool newCommandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    const VkCommandBuffer newCommandBuffer = newCommandPool.allocatePrimary();
    Semaphore newImageAvailable(device.get());
    Fence newInFlight(device.get(), VK_FENCE_CREATE_SIGNALED_BIT);

    reset();
    commandPool_ = std::move(newCommandPool);
    commandBuffer_ = newCommandBuffer;
    imageAvailable_ = std::move(newImageAvailable);
    inFlight_ = std::move(newInFlight);
}

void FrameContext::reset() noexcept
{
    inFlight_.reset();
    imageAvailable_.reset();
    commandBuffer_ = VK_NULL_HANDLE;
    commandPool_.reset();
}

void FrameContext::waitUntilReusable() const
{
    inFlight_.wait();
}

void FrameContext::resetCommands()
{
    if (!commandPool_ || commandBuffer_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot reset an empty FrameContext");
    }
    commandPool_.resetCommands();
}

void FrameContext::resetFence()
{
    inFlight_.resetSignal();
}

} // namespace VkRenderer
