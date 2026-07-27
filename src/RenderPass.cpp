#include "RenderPass.h"

#include <stdexcept>
#include <utility>

namespace VkRenderer
{

RenderPass::RenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo)
{
    create(device, createInfo);
}

RenderPass::~RenderPass()
{
    reset();
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      renderPass_(std::exchange(other.renderPass_, VK_NULL_HANDLE))
{
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        renderPass_ = std::exchange(other.renderPass_, VK_NULL_HANDLE);
    }
    return *this;
}

void RenderPass::create(VkDevice device, const VkRenderPassCreateInfo& createInfo)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("cannot create RenderPass with an invalid device");
    }

    VkRenderPass newRenderPass = VK_NULL_HANDLE;
    if (vkCreateRenderPass(device, &createInfo, nullptr, &newRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }

    reset();
    device_ = device;
    renderPass_ = newRenderPass;
}

void RenderPass::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && renderPass_ != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
    }
    device_ = VK_NULL_HANDLE;
    renderPass_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
