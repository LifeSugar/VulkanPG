#pragma once

#include "Buffer.h"

#include <vulkan/vulkan.h>

namespace VkRenderer
{

// Resources selected by currentFrame. They limit how many frames the CPU may
// submit before waiting for the GPU.
struct FrameInFlight
{
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
};

// Resources selected by the imageIndex returned from vkAcquireNextImageKHR.
// Their count and lifetime follow the swapchain images.
struct SwapchainFrame
{
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    Buffer uniformBuffer;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;

    // Non-owning reference to the FrameInFlight fence currently using this image.
    VkFence imageInFlight = VK_NULL_HANDLE;
};

} // namespace VkRenderer
