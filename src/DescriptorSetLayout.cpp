#include "DescriptorSetLayout.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

DescriptorSetLayout::DescriptorSetLayout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings,
    VkDescriptorSetLayoutCreateFlags flags)
{
    create(device, bindings, flags);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    reset();
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      layout_(std::exchange(other.layout_, VK_NULL_HANDLE))
{
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        layout_ = std::exchange(other.layout_, VK_NULL_HANDLE);
    }
    return *this;
}

void DescriptorSetLayout::create(
    VkDevice device,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings,
    VkDescriptorSetLayoutCreateFlags flags)
{
    if (device == VK_NULL_HANDLE || bindings.empty())
    {
        throw std::invalid_argument(
            "cannot create a DescriptorSetLayout with an invalid device or no bindings");
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    VkDescriptorSetLayout newLayout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &newLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    reset();
    device_ = device;
    layout_ = newLayout;
}

void DescriptorSetLayout::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && layout_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
    }

    device_ = VK_NULL_HANDLE;
    layout_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
