#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

class Framebuffer final
{
public:
    Framebuffer() = default;
    Framebuffer(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& attachments,
        VkExtent2D extent,
        uint32_t layers = 1);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void create(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& attachments,
        VkExtent2D extent,
        uint32_t layers = 1);
    void reset() noexcept;

    [[nodiscard]] VkFramebuffer get() const noexcept { return framebuffer_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return framebuffer_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
