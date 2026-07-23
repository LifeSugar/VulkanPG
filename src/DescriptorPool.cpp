#include "DescriptorPool.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

DescriptorPool::DescriptorPool(
    VkDevice device,
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets,
    VkDescriptorPoolCreateFlags flags)
{
    create(device, poolSizes, maxSets, flags);
}

DescriptorPool::~DescriptorPool()
{
    reset();
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      pool_(std::exchange(other.pool_, VK_NULL_HANDLE))
{
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        pool_ = std::exchange(other.pool_, VK_NULL_HANDLE);
    }
    return *this;
}

void DescriptorPool::create(
    VkDevice device,
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets,
    VkDescriptorPoolCreateFlags flags)
{
    if (device == VK_NULL_HANDLE || poolSizes.empty() || maxSets == 0)
    {
        throw std::invalid_argument(
            "cannot create a DescriptorPool with an invalid device or empty capacity");
    }

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = maxSets;

    VkDescriptorPool newPool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &newPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    reset();
    device_ = device;
    pool_ = newPool;
}

void DescriptorPool::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device_, pool_, nullptr);
    }

    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
}

std::vector<VkDescriptorSet> DescriptorPool::allocate(
    VkDescriptorSetLayout layout,
    uint32_t count) const
{
    if (device_ == VK_NULL_HANDLE || pool_ == VK_NULL_HANDLE)
    {
        throw std::logic_error("cannot allocate descriptor sets from an empty DescriptorPool");
    }
    if (layout == VK_NULL_HANDLE || count == 0)
    {
        throw std::invalid_argument("descriptor set allocation requires a layout and non-zero count");
    }

    const std::vector<VkDescriptorSetLayout> layouts(count, layout);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pool_;
    allocateInfo.descriptorSetCount = count;
    allocateInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(count);
    if (vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    return descriptorSets;
}

} // namespace VkRenderer
