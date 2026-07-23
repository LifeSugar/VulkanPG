#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

class DescriptorPool final
{
public:
    DescriptorPool() = default;
    DescriptorPool(
        VkDevice device,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags = 0);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    DescriptorPool(DescriptorPool&& other) noexcept;
    DescriptorPool& operator=(DescriptorPool&& other) noexcept;

    void create(
        VkDevice device,
        const std::vector<VkDescriptorPoolSize>& poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags = 0);
    void reset() noexcept;

    [[nodiscard]] std::vector<VkDescriptorSet> allocate(
        VkDescriptorSetLayout layout,
        uint32_t count) const;

    [[nodiscard]] VkDescriptorPool get() const noexcept { return pool_; }
    [[nodiscard]] explicit operator bool() const noexcept { return pool_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
