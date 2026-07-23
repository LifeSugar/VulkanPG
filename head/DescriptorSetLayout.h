#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

class DescriptorSetLayout final
{
public:
    DescriptorSetLayout() = default;
    DescriptorSetLayout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0);
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

    void create(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0);
    void reset() noexcept;

    [[nodiscard]] VkDescriptorSetLayout get() const noexcept { return layout_; }
    [[nodiscard]] explicit operator bool() const noexcept { return layout_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
