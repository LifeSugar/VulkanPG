#pragma once

#include "Device.h"

#include <vector>

namespace VkRenderer
{

class CommandPool final
{
public:
    CommandPool() = default;
    CommandPool(
        const Device& device,
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0);
    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&& other) noexcept;
    CommandPool& operator=(CommandPool&& other) noexcept;

    void create(
        const Device& device,
        uint32_t queueFamilyIndex,
        VkCommandPoolCreateFlags flags = 0);
    void resetCommands(VkCommandPoolResetFlags flags = 0) const;
    void reset() noexcept;

    [[nodiscard]] VkCommandBuffer allocatePrimary() const;
    [[nodiscard]] std::vector<VkCommandBuffer> allocatePrimary(uint32_t count) const;
    void free(VkCommandBuffer commandBuffer) const noexcept;
    void free(const std::vector<VkCommandBuffer>& commandBuffers) const noexcept;

    [[nodiscard]] VkCommandPool get() const noexcept { return commandPool_; }
    [[nodiscard]] explicit operator bool() const noexcept { return commandPool_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
