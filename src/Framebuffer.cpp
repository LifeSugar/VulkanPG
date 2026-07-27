#include "Framebuffer.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

Framebuffer::Framebuffer(
    VkDevice device,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& attachments,
    VkExtent2D extent,
    uint32_t layers)
{
    create(device, renderPass, attachments, extent, layers);
}

Framebuffer::~Framebuffer()
{
    reset();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      framebuffer_(std::exchange(other.framebuffer_, VK_NULL_HANDLE))
{
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        framebuffer_ = std::exchange(other.framebuffer_, VK_NULL_HANDLE);
    }
    return *this;
}

void Framebuffer::create(
    VkDevice device,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& attachments,
    VkExtent2D extent,
    uint32_t layers)
{
    if (device == VK_NULL_HANDLE ||
        renderPass == VK_NULL_HANDLE ||
        attachments.empty() ||
        extent.width == 0 ||
        extent.height == 0 ||
        layers == 0)
    {
        throw std::invalid_argument("framebuffer create info is incomplete");
    }

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    createInfo.layers = layers;

    VkFramebuffer newFramebuffer = VK_NULL_HANDLE;
    if (vkCreateFramebuffer(device, &createInfo, nullptr, &newFramebuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }

    reset();
    device_ = device;
    framebuffer_ = newFramebuffer;
}

void Framebuffer::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && framebuffer_ != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    framebuffer_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
