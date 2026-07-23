#include "App.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

Swapchain App::makeSwapChain(VkSwapchainKHR oldSwapChain) const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    const VkExtent2D framebufferExtent = {
        static_cast<uint32_t>(std::max(width, 0)),
        static_cast<uint32_t>(std::max(height, 0))
    };

    return Swapchain(
        device.physical(),
        device.get(),
        surface,
        device.graphicsQueueFamily(),
        device.presentQueueFamily(),
        framebufferExtent,
        oldSwapChain);
}

void App::createDepthResources()
{
    const VkFormat depthFormat = findDepthFormat();
    const VkExtent2D extent = swapChain.extent();
    swapchainFrames.clear();
    swapchainFrames.resize(swapChain.imageCount());

    for (SwapchainFrame& frame : swapchainFrames)
    {
        frame.depthImage.create(
            device.physical(),
            device.get(),
            extent.width,
            extent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        frame.depthImageView.create(
            device.get(),
            frame.depthImage.get(),
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }
}

void App::createFramebuffers()
{
    const std::vector<ImageView>& imageViews = swapChain.imageViews();
    const VkExtent2D extent = swapChain.extent();
    if (swapchainFrames.size() != imageViews.size())
    {
        throw std::runtime_error("swapchain frame resources do not match swapchain images");
    }

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        const std::array<VkImageView, 2> attachments = {
            imageViews[i].get(),
            swapchainFrames[i].depthImageView.get()
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
                device.get(),
                &framebufferInfo,
                nullptr,
                &swapchainFrames[i].framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

void App::requestSwapChainRecreation()
{
    swapChainRecreationRequested = true;
    lastFramebufferResizeTime = glfwGetTime();
}

bool App::isSwapChainRecreationDue() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    return width > 0 && height > 0 &&
        glfwGetTime() - lastFramebufferResizeTime >= kSwapChainResizeDebounceSeconds;
}

void App::cleanupSwapChainDependents()
{
    // Callers wait for the device to become idle before reaching this point.
    // Reset the per-frame pools so their command buffers stop referencing old
    // swapchain framebuffers, descriptors, render passes, and pipelines.
    for (FrameInFlight& frame : framesInFlight)
    {
        if (frame.commandPool)
        {
            frame.commandPool.resetCommands();
        }
    }

    for (SwapchainFrame& frame : swapchainFrames)
    {
        if (frame.framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device.get(), frame.framebuffer, nullptr);
            frame.framebuffer = VK_NULL_HANDLE;
        }
        if (frame.renderFinished != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device.get(), frame.renderFinished, nullptr);
            frame.renderFinished = VK_NULL_HANDLE;
        }
        frame.imageInFlight = VK_NULL_HANDLE;
    }

    if (graphicPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device.get(), graphicPipeline, nullptr);
        graphicPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device.get(), pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device.get(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    swapchainFrames.clear();
}

void App::cleanupSwapChain()
{
    cleanupSwapChainDependents();
    swapChain.reset();
}

void App::recreateSwapChain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    device.waitIdle();

    // Construct the replacement first. If creation fails, the current swapchain
    // and all of its dependent resources remain intact.
    Swapchain newSwapChain = makeSwapChain(swapChain.get());
    cleanupSwapChainDependents();
    swapChain = std::move(newSwapChain);

    // The projection matrix must use the new render extent; otherwise a resize
    // changes the viewport without changing the camera aspect ratio.
    const VkExtent2D extent = swapChain.extent();
    camera.setAspect(
        static_cast<float>(extent.width) /
        static_cast<float>(extent.height));

    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createSwapchainFrameSyncObjects();

    swapChainRecreationRequested = false;
}

} // namespace VkRenderer
