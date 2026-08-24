#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan render pass.
class RenderPass final
{
public:
    /// Creates an empty render-pass wrapper.
    RenderPass() = default;
    /// Creates a render pass from Vulkan creation parameters.
    RenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo);
    /// Destroys the owned render pass.
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    /// Transfers render-pass ownership from another wrapper.
    RenderPass(RenderPass&& other) noexcept;
    /// Replaces this render pass by taking ownership from another wrapper.
    RenderPass& operator=(RenderPass&& other) noexcept;

    /// Creates or replaces the owned render pass.
    void create(VkDevice device, const VkRenderPassCreateInfo& createInfo);
    /// Destroys the owned render pass and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan render-pass handle.
    [[nodiscard]] VkRenderPass get() const noexcept { return renderPass_; }
    /// Returns whether a render pass is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return renderPass_ != VK_NULL_HANDLE;
    }

private:
    /// Logical device that owns the render pass.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan render-pass handle.
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
