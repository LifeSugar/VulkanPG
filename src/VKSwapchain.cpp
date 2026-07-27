#include "App.h"

#include <algorithm>

namespace VkRenderer
{

VkExtent2D App::framebufferExtent() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    return {
        static_cast<uint32_t>(std::max(width, 0)),
        static_cast<uint32_t>(std::max(height, 0))
    };
}

void App::requestSwapChainRecreation()
{
    swapChainRecreationRequested = true;
    lastFramebufferResizeTime = glfwGetTime();
}

bool App::isSwapChainRecreationDue() const
{
    const VkExtent2D extent = framebufferExtent();
    return extent.width > 0 &&
        extent.height > 0 &&
        glfwGetTime() - lastFramebufferResizeTime >=
            kSwapChainResizeDebounceSeconds;
}

void App::recreateSwapChain()
{
    VkExtent2D extent = framebufferExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        glfwWaitEvents();
        extent = framebufferExtent();
    }

    device.waitIdle();

    // Recorded commands reference old framebuffers/render passes. Release those
    // references before SwapchainResources commits the replacement bundle.
    for (FrameContext& frame : frameContexts)
    {
        frame.resetCommands();
    }

    SwapchainResources::CreateInfo createInfo{};
    createInfo.surface = surface;
    createInfo.framebufferExtent = extent;
    const bool pipelineCompatibilityChanged =
        swapchainResources.recreate(device, createInfo);
    if (pipelineCompatibilityChanged)
    {
        graphicsPipeline.create(device, makeGraphicsPipelineCreateInfo());
    }

    const VkExtent2D renderExtent = swapchainResources.extent();
    camera.setAspect(
        static_cast<float>(renderExtent.width) /
        static_cast<float>(renderExtent.height));

    swapChainRecreationRequested = false;
}

} // namespace VkRenderer
