#include "Sampler.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

Sampler::Sampler(VkDevice device, const VkSamplerCreateInfo& createInfo)
{
    create(device, createInfo);
}

Sampler::~Sampler()
{
    reset();
}

Sampler::Sampler(Sampler&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      sampler_(std::exchange(other.sampler_, VK_NULL_HANDLE))
{
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        sampler_ = std::exchange(other.sampler_, VK_NULL_HANDLE);
    }
    return *this;
}

void Sampler::create(
    VkDevice device,
    const VkSamplerCreateInfo& createInfo)
{
    if (device == VK_NULL_HANDLE ||
        createInfo.sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
    {
        throw std::invalid_argument("sampler create info is incomplete");
    }

    VkSampler newSampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device, &createInfo, nullptr, &newSampler) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create sampler");
    }

    reset();
    device_ = device;
    sampler_ = newSampler;
}

void Sampler::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, sampler_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    sampler_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
