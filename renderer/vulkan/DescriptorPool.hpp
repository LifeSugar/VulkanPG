#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan descriptor pool.
class DescriptorPool final
{
public:
    /// Creates an empty descriptor-pool wrapper.
    DescriptorPool() = default;
    /// Creates a descriptor pool with the requested capacities.
    DescriptorPool(
        VkDevice device,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags = 0);
    /// Destroys the descriptor pool and its descriptor sets.
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    /// Transfers descriptor-pool ownership from another wrapper.
    DescriptorPool(DescriptorPool&& other) noexcept;
    /// Replaces this pool by taking ownership from another wrapper.
    DescriptorPool& operator=(DescriptorPool&& other) noexcept;

    /// Creates or replaces the descriptor pool.
    void create(
        VkDevice device,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags = 0);
    /// Destroys the descriptor pool and clears its state.
    void reset() noexcept;

    /// Allocates descriptor sets that all use the supplied layout.
    [[nodiscard]] std::vector<VkDescriptorSet> allocate(
        VkDescriptorSetLayout layout,
        uint32_t count) const;

    /// Returns the owned Vulkan descriptor-pool handle.
    [[nodiscard]] VkDescriptorPool get() const noexcept { return pool_; }
    /// Returns whether a descriptor pool is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept { return pool_ != VK_NULL_HANDLE; }

private:
    /// Logical device that owns the descriptor pool.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan descriptor-pool handle.
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
