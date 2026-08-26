#include "App.hpp"

#include "ApplicationGui.hpp"
#include "content/DemoContent.hpp"
#include "vulkan/DefaultPipelineFactory.hpp"
#include "render/RenderFrameBuilder.hpp"
#include "RuntimeGui.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VkRenderer
{

App::~App()
{
    discardTextureReimport();
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
    RuntimeGui gui;
    run(RunConfig{}, gui);
}

void App::run(const RunConfig& config, ApplicationGui& gui)
{
    initWindow(config, true);
    initVulkan(config);
    initImGui(config);
    guiRenderBridge.attach(renderer, renderAssets);
    ApplicationGuiContext guiContext{
        assetManager,
        scene,
        guiRenderBridge,
        &textureImports};
    gui.attach(guiContext);
    try
    {
        mainLoop(gui);
    }
    catch (...)
    {
        renderer.waitIdle();
        gui.detach();
        cleanup();
        throw;
    }
    renderer.waitIdle();
    gui.detach();
    cleanup();
}

void App::initWindow(const RunConfig& config, bool visible)
{
    if (config.windowWidth == 0 || config.windowHeight == 0 ||
        config.windowTitle.empty())
    {
        throw std::invalid_argument("App run config contains an invalid window");
    }

    Window::CreateInfo createInfo{};
    createInfo.width = config.windowWidth;
    createInfo.height = config.windowHeight;
    createInfo.title = config.windowTitle;
    createInfo.visible = visible;
    window.create(createInfo);
}

void App::initVulkan(const RunConfig& config)
{
    VulkanContext::CreateInfo contextCreateInfo{};
    contextCreateInfo.enableValidationLayers = kEnableValidationLayers;
    contextCreateInfo.preferIntegratedGpu = preferIntegratedGpu;
    vulkanContext.create(window, contextCreateInfo);

    demoContent = DemoContentLoader::load(
        assetManager,
        scene,
        config.demoContent,
        &textureImports);

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
        {demoContent.model});

    VulkanRenderer::CreateInfo rendererCreateInfo{};
    rendererCreateInfo.context = &vulkanContext;
    rendererCreateInfo.framebufferExtent = window.framebufferExtent();
    rendererCreateInfo.framesInFlight = kMaxFramesInFlight;
    rendererCreateInfo.outputMode = config.outputMode;
    rendererCreateInfo.graphicsPipeline = makeDefaultScenePipeline(
        assetManager.shader(demoContent.pbrVertexShader),
        assetManager.shader(demoContent.pbrFragmentShader),
        renderAssets.materialDescriptorSetLayout());
    rendererCreateInfo.presentPipeline = makeDefaultPresentPipeline(
        assetManager.shader(demoContent.presentVertexShader),
        assetManager.shader(demoContent.presentFragmentShader));
    renderer.create(rendererCreateInfo);

    setupCamera();
}

void App::cleanup()
{
    discardTextureReimport();
    renderer.waitIdle();
    guiRenderBridge.detach();
    imguiLayer.reset();
    renderer.reset();
    renderAssets.reset();
    scene.reset();
    textureImports.reset();
    assetManager.reset();
    demoContent = {};
    vulkanContext.reset();
    window.reset();
}

void App::initImGui(const RunConfig& config)
{
    ImGuiLayer::CreateInfo createInfo{};
    createInfo.window = &window;
    createInfo.context = &vulkanContext;
    createInfo.renderPass = renderer.presentRenderPass();
    createInfo.minImageCount = 2;
    createInfo.imageCount = renderer.swapchainImageCount();
    createInfo.enableDocking = config.enableDocking;
    createInfo.iniFilename = config.imguiIniFilename;
    imguiLayer.create(createInfo);
}

void App::setupCamera()
{
    camera.setPosition(glm::vec3(0.0f, 1.0f, 0.5f));
    camera.setRotation(glm::vec3(-60.0f, 0.0f, 0.0f));
    const VkExtent2D extent = renderer.extent();
    camera.setAspect(
        static_cast<float>(extent.width) /
        static_cast<float>(extent.height));
}

void App::mainLoop(ApplicationGui& gui)
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
                // During an interactive resize, avoid repeatedly destroying and
                // recreating GPU resources.
                window.waitEventsTimeout(0.016);
                continue;
            }

            recreateSwapChain(gui);
        }

        processPendingTextureReimport();

        imguiLayer.beginFrame();
        drawGui(gui);
        ImDrawData* uiDrawData = imguiLayer.endFrame();

        const VulkanRenderer::RenderResult renderResult =
            renderer.render(makeRenderFrame(), renderAssets, uiDrawData);
        if (renderResult == VulkanRenderer::RenderResult::NeedsResize)
        {
            requestSwapChainRecreation();
        }
    }
}

void App::drawGui(ApplicationGui& gui)
{
    ApplicationGuiContext context{
        assetManager,
        scene,
        guiRenderBridge,
        &textureImports};
    const ApplicationGuiFrameOutput output = gui.draw(context);
    for (const TextureReimportRequest& request : output.textureReimports)
    {
        const bool alreadyQueued = std::any_of(
            pendingTextureReimports_.begin(),
            pendingTextureReimports_.end(),
            [&](const TextureReimportRequest& queued)
            {
                return queued.texture == request.texture;
            });
        if (!alreadyQueued && activeTextureReimport_ != request.texture)
        {
            pendingTextureReimports_.push_back(request);
        }
    }
    if (output.sceneAspectRatio)
    {
        const float aspect = *output.sceneAspectRatio;
        if (!std::isfinite(aspect) || aspect <= 0.0f)
        {
            throw std::invalid_argument(
                "Application GUI returned an invalid scene aspect ratio");
        }
        camera.setAspect(aspect);
    }
}

RenderFrame App::makeRenderFrame()
{
    camera.Update();
    return buildRenderFrame(
        scene,
        assetManager,
        camera.makeRenderView());
}

} // namespace VkRenderer
