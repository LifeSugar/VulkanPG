#include "App.h"

namespace VkRenderer
{

void App::requestSwapChainRecreation()
{
    swapChainRecreationRequested = true;
#ifndef __ANDROID__
    lastFramebufferResizeTime = Window::time();
#endif
}

bool App::isSwapChainRecreationDue() const
{
#ifdef __ANDROID__
    return pendingResizeExtent_.width > 0 &&
        pendingResizeExtent_.height > 0;
#else
    const VkExtent2D extent = window.framebufferExtent();
    return extent.width > 0 &&
        extent.height > 0 &&
        Window::time() - lastFramebufferResizeTime >=
            kSwapChainResizeDebounceSeconds;
#endif
}

void App::recreateSwapChain()
{
#ifdef __ANDROID__
    VkExtent2D extent = pendingResizeExtent_;
    pendingResizeExtent_ = {};
#else
    VkExtent2D extent = window.framebufferExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        window.waitEvents();
        extent = window.framebufferExtent();
    }
#endif

    renderer.resize(extent);

    const VkExtent2D renderExtent = renderer.extent();
    camera.setAspect(
        static_cast<float>(renderExtent.width) /
        static_cast<float>(renderExtent.height));

    swapChainRecreationRequested = false;
}

void App::resizeSwapchain(VkExtent2D newExtent)
{
    pendingResizeExtent_ = newExtent;
    requestSwapChainRecreation();
}

} // namespace VkRenderer
