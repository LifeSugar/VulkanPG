#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

class RenderPass final
{
public:
    RenderPass() = default;
    RenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&& other) noexcept;
    RenderPass& operator=(RenderPass&& other) noexcept;

    void create(VkDevice device, const VkRenderPassCreateInfo& createInfo);
    void reset() noexcept;

    [[nodiscard]] VkRenderPass get() const noexcept { return renderPass_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return renderPass_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
