#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan sampler.
class Sampler final
{
public:
    Sampler() = default;
    Sampler(VkDevice device, const VkSamplerCreateInfo& createInfo);
    ~Sampler();

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;

    void create(VkDevice device, const VkSamplerCreateInfo& createInfo);
    void reset() noexcept;

    [[nodiscard]] VkSampler get() const noexcept { return sampler_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return sampler_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
