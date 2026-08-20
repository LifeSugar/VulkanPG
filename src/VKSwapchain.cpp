#include "App.h"

namespace VkRenderer
{

void App::requestSwapChainRecreation()
{
    swapChainRecreationRequested = true;
    lastFramebufferResizeTime = Window::time();
}

bool App::isSwapChainRecreationDue() const
{
    const VkExtent2D extent = window.framebufferExtent();
    return extent.width > 0 &&
        extent.height > 0 &&
        Window::time() - lastFramebufferResizeTime >=
            kSwapChainResizeDebounceSeconds;
}

void App::recreateSwapChain()
{
    VkExtent2D extent = window.framebufferExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        window.waitEvents();
        extent = window.framebufferExtent();
    }

    const bool recreateImGui = static_cast<bool>(imguiLayer);
    renderer.resize(extent);

    if (recreateImGui)
    {
        imguiLayer.recreateRendererPipeline(
            renderer.presentRenderPass());
    }

    const VkExtent2D renderExtent = renderer.extent();
    camera.setAspect(
        static_cast<float>(renderExtent.width) /
        static_cast<float>(renderExtent.height));

    swapChainRecreationRequested = false;
}

} // namespace VkRenderer
