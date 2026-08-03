#include "App.h"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

namespace VkRenderer
{

App::~App()
{
    // Destruction after an exception must not release resources still in use by
    // the GPU. Member destruction then proceeds in reverse dependency order.
    vulkanContext_.waitIdle();
}

void App::setPreferIntegratedGPU(bool enabled)
{
    preferIntegratedGpu = enabled;
}

#ifndef __ANDROID__
void App::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}
#endif

void App::initWindow()
{
#ifndef __ANDROID__
    Window::CreateInfo createInfo{};
    createInfo.width = kWindowWidth;
    createInfo.height = kWindowHeight;
    createInfo.title = "Vulkan";
    window.create(createInfo);
#endif
}

#ifndef __ANDROID__
void App::initVulkan()
{
    VulkanContext::CreateInfo contextCreateInfo{};
    contextCreateInfo.enableValidationLayers = kEnableValidationLayers;
    contextCreateInfo.preferIntegratedGpu = preferIntegratedGpu;
    vulkanContext_.create(window, contextCreateInfo);

    VulkanRenderer::CreateInfo rendererCreateInfo{};
    rendererCreateInfo.context = &vulkanContext_;
    rendererCreateInfo.framebufferExtent = window.framebufferExtent();
    rendererCreateInfo.framesInFlight = kMaxFramesInFlight;
    rendererCreateInfo.graphicsPipeline = makeGraphicsPipelineCreateInfo();
    renderer.create(rendererCreateInfo);

    initVulkanCommon(window.framebufferExtent());
}
#endif

void App::initVulkan(
    VkSurfaceKHR surface,
    uint32_t width,
    uint32_t height)
{
    vulkanContext_.initSurfaceAndDevice(surface, preferIntegratedGpu);

    const VkExtent2D extent = {width, height};

    VulkanRenderer::CreateInfo rendererCreateInfo{};
    rendererCreateInfo.context = &vulkanContext_;
    rendererCreateInfo.framebufferExtent = extent;
    rendererCreateInfo.framesInFlight = kMaxFramesInFlight;
    rendererCreateInfo.graphicsPipeline = makeGraphicsPipelineCreateInfo();
    renderer.create(rendererCreateInfo);

    initVulkanCommon(extent);
}

void App::initVulkanCommon(VkExtent2D framebufferExtent)
{
    setupCamera();
    const MeshData meshData = loadModel();
    const Device& device = vulkanContext_.device();
    CommandPool uploadCommandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    UploadContext uploadContext(device, uploadCommandPool);
    mesh.create(uploadContext, meshData);
}

void App::cleanup()
{
    renderer.reset();
    mesh.reset();
    vulkanContext_.reset();
#ifndef __ANDROID__
    window.reset();
#endif
}

void App::renderFrame()
{
    const VulkanRenderer::RenderResult renderResult =
        renderer.render(makeRenderFrame());
    if (renderResult == VulkanRenderer::RenderResult::NeedsResize)
    {
        requestSwapChainRecreation();
    }
}

void App::setupCamera()
{
    camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    const VkExtent2D extent = renderer.extent();
    camera.setAspect(static_cast<float>(extent.width) / static_cast<float>(extent.height));
}

#ifndef __ANDROID__
void App::mainLoop()
{
    while (!window.shouldClose())
    {
        window.pollEvents();

        if (window.consumeFramebufferResize())
        {
            requestSwapChainRecreation();
        }

        if (swapChainRecreationRequested)
        {
            if (!isSwapChainRecreationDue())
            {
                // During an interactive resize, avoid repeatedly destroying and recreating GPU resources.
                window.waitEventsTimeout(0.016);
                continue;
            }

            recreateSwapChain();
        }

        const VulkanRenderer::RenderResult renderResult =
            renderer.render(makeRenderFrame());
        if (renderResult == VulkanRenderer::RenderResult::NeedsResize)
        {
            requestSwapChainRecreation();
        }
    }
}
#endif

RenderFrame App::makeRenderFrame()
{
    static const auto startTime =
        std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<
        float,
        std::chrono::seconds::period>(currentTime - startTime).count();

    camera.Update();

    RenderFrame frame{};
    frame.view.cameraData = camera.getGpuData();
    frame.view.cameraRevision = camera.revision();

    RenderObject object{};
    object.mesh = &mesh;
    object.objectData.world = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(45.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    object.objectData.normalMatrix =
        glm::transpose(glm::inverse(object.objectData.world));
    frame.objects.push_back(object);
    return frame;
}

} // namespace VkRenderer
