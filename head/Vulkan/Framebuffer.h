#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

/// RAII wrapper for a Vulkan framebuffer.
class Framebuffer final
{
public:
    /// Creates an empty framebuffer wrapper.
    Framebuffer() = default;
    /// Creates a framebuffer for the supplied render pass and attachments.
    Framebuffer(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& attachments,
        VkExtent2D extent,
        uint32_t layers = 1);
    /// Destroys the owned framebuffer.
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    /// Transfers framebuffer ownership from another wrapper.
    Framebuffer(Framebuffer&& other) noexcept;
    /// Replaces this framebuffer by taking ownership from another wrapper.
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /// Creates or replaces the owned framebuffer.
    void create(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& attachments,
        VkExtent2D extent,
        uint32_t layers = 1);
    /// Destroys the owned framebuffer and clears its state.
    void reset() noexcept;

    /// Returns the owned Vulkan framebuffer handle.
    [[nodiscard]] VkFramebuffer get() const noexcept { return framebuffer_; }
    /// Returns whether a framebuffer is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return framebuffer_ != VK_NULL_HANDLE;
    }

private:
    /// Logical device that owns the framebuffer.
    VkDevice device_ = VK_NULL_HANDLE;
    /// Owned Vulkan framebuffer handle.
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
