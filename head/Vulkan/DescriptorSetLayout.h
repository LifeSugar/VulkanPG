#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan descriptor set layout.
class DescriptorSetLayout final
{
public:
    /// Creates an empty descriptor-set-layout wrapper.
    DescriptorSetLayout() = default;
    /// Creates a descriptor set layout from the supplied bindings.
    DescriptorSetLayout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0);
    /// Destroys the owned descriptor set layout.
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    /// Transfers layout ownership from another wrapper.
    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    /// Replaces this layout by taking ownership from another wrapper.
    DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

    /// Creates or replaces the descriptor set layout.
    void create(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0);
    /// Destroys the descriptor set layout and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan descriptor-set-layout handle.
    [[nodiscard]] VkDescriptorSetLayout get() const noexcept { return layout_; }
    /// Returns whether a descriptor set layout is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept { return layout_ != VK_NULL_HANDLE; }

private:
    /// Logical device that owns the descriptor set layout.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan descriptor-set-layout handle.
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
