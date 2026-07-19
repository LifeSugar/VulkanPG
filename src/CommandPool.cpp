#include "CommandPool.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

CommandPool::CommandPool(
    const Device& device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags flags)
{
    create(device, queueFamilyIndex, flags);
}

CommandPool::~CommandPool()
{
    reset();
}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      commandPool_(std::exchange(other.commandPool_, VK_NULL_HANDLE))
{
}

CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        commandPool_ = std::exchange(other.commandPool_, VK_NULL_HANDLE);
    }
    return *this;
}

void CommandPool::create(
    const Device& device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags flags)
{
    if (!device)
    {
        throw std::invalid_argument("cannot create a CommandPool with an invalid device");
    }

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.queueFamilyIndex = queueFamilyIndex;

    VkCommandPool newCommandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device.get(), &createInfo, nullptr, &newCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }

    reset();
    device_ = device.get();
    commandPool_ = newCommandPool;
}

void CommandPool::resetCommands(VkCommandPoolResetFlags flags) const
{
    if (commandPool_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot reset an empty CommandPool");
    }
    if (vkResetCommandPool(device_, commandPool_, flags) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to reset command pool");
    }
}

void CommandPool::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && commandPool_ != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    commandPool_ = VK_NULL_HANDLE;
}

VkCommandBuffer CommandPool::allocatePrimary() const
{
    std::vector<VkCommandBuffer> commandBuffers = allocatePrimary(1);
    return commandBuffers.front();
}

std::vector<VkCommandBuffer> CommandPool::allocatePrimary(uint32_t count) const
{
    if (commandPool_ == VK_NULL_HANDLE || count == 0)
    {
        throw std::invalid_argument("cannot allocate command buffers from an empty pool or with zero count");
    }

    VkCommandBufferAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocationInfo.commandPool = commandPool_;
    allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocationInfo.commandBufferCount = count;

    std::vector<VkCommandBuffer> commandBuffers(count);
    if (vkAllocateCommandBuffers(device_, &allocationInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers");
    }
    return commandBuffers;
}

void CommandPool::free(VkCommandBuffer commandBuffer) const noexcept
{
    if (commandPool_ != VK_NULL_HANDLE && commandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
    }
}

void CommandPool::free(const std::vector<VkCommandBuffer>& commandBuffers) const noexcept
{
    if (commandPool_ != VK_NULL_HANDLE && !commandBuffers.empty())
    {
        vkFreeCommandBuffers(
            device_,
            commandPool_,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
    }
}

} // namespace VkRenderer
