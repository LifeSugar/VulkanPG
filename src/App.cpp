#include "App.h"

#include "Render/SceneRenderExtractor.h"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

namespace VkRenderer
{

App::~App()
{
    // Destruction after an exception must not release resources still in use by
    // the GPU. Member destruction then proceeds in reverse dependency order.
    vulkanContext.waitIdle();
}

void App::setPreferIntegratedGPU(bool enabled)
{
    preferIntegratedGpu = enabled;
}

void App::run()
{
    initWindow(true);
    initVulkan();
    mainLoop();
    cleanup();
}

void App::runAssetImportTest()
{
    createDemoAssets();
}

void App::runRenderTest()
{
    initWindow(false);
    initVulkan();
    for (uint32_t frame = 0; frame < 3; ++frame)
    {
        window.pollEvents();
        const RenderFrame renderFrame = makeRenderFrame();
        if (renderFrame.objects.empty())
        {
            throw std::runtime_error(
                "scene extraction produced no render objects");
        }
        if (frame == 0)
        {
            std::clog
                << "[Render] Extracted submesh draws="
                << renderFrame.objects.size()
                << '\n';
        }
        if (renderer.render(renderFrame) ==
            VulkanRenderer::RenderResult::NeedsResize)
        {
            throw std::runtime_error(
                "hidden render test unexpectedly requires a resize");
        }
    }
    cleanup();
}

void App::initWindow(bool visible)
{
    Window::CreateInfo createInfo{};
    createInfo.width = kWindowWidth;
    createInfo.height = kWindowHeight;
    createInfo.title = "Vulkan";
    createInfo.visible = visible;
    window.create(createInfo);
}

void App::initVulkan()
{
    VulkanContext::CreateInfo contextCreateInfo{};
    contextCreateInfo.enableValidationLayers = kEnableValidationLayers;
    contextCreateInfo.preferIntegratedGpu = preferIntegratedGpu;
    vulkanContext.create(window, contextCreateInfo);

    createDemoAssets();

    const Device& device = vulkanContext.device();
    CommandPool uploadCommandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    UploadContext uploadContext(device, uploadCommandPool);
    renderAssets.create(
        device,
        uploadContext,
        assetManager,
        {demoModelAsset});

    VulkanRenderer::CreateInfo rendererCreateInfo{};
    rendererCreateInfo.context = &vulkanContext;
    rendererCreateInfo.framebufferExtent = window.framebufferExtent();
    rendererCreateInfo.framesInFlight = kMaxFramesInFlight;
    rendererCreateInfo.graphicsPipeline = makeGraphicsPipelineCreateInfo();
    rendererCreateInfo.presentPipeline = makePresentPipelineCreateInfo();
    renderer.create(rendererCreateInfo);

    setupCamera();
}

void App::cleanup()
{
    renderer.reset();
    renderAssets.reset();
    scene.reset();
    assetManager.reset();
    demoTextureAsset = {};
    pbrVertexShaderAsset = {};
    pbrFragmentShaderAsset = {};
    presentVertexShaderAsset = {};
    presentFragmentShaderAsset = {};
    demoMaterialTemplateAsset = {};
    demoMaterialAsset = {};
    demoModelAsset = {};
    vulkanContext.reset();
    window.reset();
}

void App::setupCamera()
{
    camera.setPosition(glm::vec3(0.0f, 1.0f, 0.5f));
    camera.setRotation(glm::vec3(-60.0f, 0.0f, 0.0f));
    const VkExtent2D extent = renderer.extent();
    camera.setAspect(static_cast<float>(extent.width) / static_cast<float>(extent.height));
}

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

RenderFrame App::makeRenderFrame()
{
    static const auto startTime =
        std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<
        float,
        std::chrono::seconds::period>(currentTime - startTime).count();

    camera.Update();

    scene.setLocalTransform(
        0,
        glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(45.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)));

    RenderView view{};
    view.cameraData = camera.getGpuData();
    view.cameraRevision = camera.revision();
    return SceneRenderExtractor{}.extract(
        scene,
        assetManager,
        renderAssets,
        view);
}

} // namespace VkRenderer
