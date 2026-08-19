#include "App.h"

#include "Render/CullingSystem.h"
#include "Render/MaterialKey.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderItemComparator.h"
#include "Render/RenderListBuilder.h"
#include "Render/SceneRenderExtractor.h"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

void validateRenderKeys()
{
    constexpr MaterialKey olderMaterial =
        makeMaterialKey(MaterialAssetHandle{4, 1});
    constexpr MaterialKey newerMaterial =
        makeMaterialKey(MaterialAssetHandle{4, 2});
    if (!MaterialKeyLess{}(olderMaterial, newerMaterial) ||
        MaterialKeyLess{}(newerMaterial, olderMaterial))
    {
        throw std::runtime_error(
            "MaterialKey comparison is not deterministic");
    }

    MaterialRenderState opaqueState = makeOpaqueMaterialState();
    const PipelineVariantKey opaque = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        opaqueState);

    MaterialRenderState alphaClipState = opaqueState;
    alphaClipState.alphaClipEnabled = true;
    const PipelineVariantKey alphaClip = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        alphaClipState);
    alphaClipState.alphaClipThreshold = 0.25f;
    const PipelineVariantKey alphaClipWithDifferentThreshold =
        makePipelineVariantKey(
            MaterialTemplateAssetHandle{1, 1},
            alphaClipState);
    if (alphaClip != alphaClipWithDifferentThreshold)
    {
        throw std::runtime_error(
            "alpha-clip threshold incorrectly changes PipelineVariantKey");
    }

    MaterialRenderState transparentState =
        makeTransparentMaterialState();
    const PipelineVariantKey transparent = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        transparentState);

    if (opaque == alphaClip || opaque == transparent ||
        !PipelineVariantKeyLess{}(opaque, alphaClip) ||
        PipelineVariantKeyLess{}(alphaClip, opaque) ||
        !PipelineVariantKeyLess{}(alphaClip, transparent) ||
        PipelineVariantKeyLess{}(transparent, alphaClip))
    {
        throw std::runtime_error(
            "PipelineVariantKey comparison produced an invalid order");
    }

    MaterialRenderState depthDisabledA = opaqueState;
    depthDisabledA.depth.testEnabled = false;
    depthDisabledA.depth.writeEnabled = false;
    depthDisabledA.depth.compare = DepthCompare::Never;
    MaterialRenderState depthDisabledB = depthDisabledA;
    depthDisabledB.depth.compare = DepthCompare::Greater;
    if (makePipelineVariantKey(
            MaterialTemplateAssetHandle{1, 1},
            depthDisabledA) !=
        makePipelineVariantKey(
            MaterialTemplateAssetHandle{1, 1},
            depthDisabledB))
    {
        throw std::runtime_error(
            "ignored depth compare operation was not canonicalized");
    }
}

void validateRenderItemComparators()
{
    if (Detail::opaqueDepthSortBucket(0.125f) !=
            Detail::opaqueDepthSortBucket(0.499f) ||
        Detail::opaqueDepthSortBucket(0.5f) !=
            Detail::opaqueDepthSortBucket(1.999f) ||
        Detail::opaqueDepthSortBucket(2.0f) !=
            Detail::opaqueDepthSortBucket(7.999f) ||
        Detail::opaqueDepthSortBucket(8.0f) !=
            Detail::opaqueDepthSortBucket(31.999f) ||
        Detail::opaqueDepthSortBucket(0.499f) >=
            Detail::opaqueDepthSortBucket(0.5f) ||
        Detail::opaqueDepthSortBucket(1.999f) >=
            Detail::opaqueDepthSortBucket(2.0f) ||
        Detail::opaqueDepthSortBucket(7.999f) >=
            Detail::opaqueDepthSortBucket(8.0f))
    {
        throw std::runtime_error(
            "opaque depth bucket quantization produced invalid boundaries");
    }

    RenderItem regularNear{};
    regularNear.materialHandle = MaterialAssetHandle{1, 1};
    regularNear.materialKey = makeMaterialKey(regularNear.materialHandle);
    regularNear.meshHandle = MeshAssetHandle{1, 1};
    regularNear.pipelineKey = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        makeOpaqueMaterialState());
    regularNear.queue = RenderQueue::Opaque;
    regularNear.viewDepth = 2.0f;
    regularNear.candidateIndex = 2;

    RenderItem regularFar = regularNear;
    regularFar.viewDepth = 10.0f;
    regularFar.candidateIndex = 3;

    RenderItem lowerMaterial = regularNear;
    lowerMaterial.materialHandle = MaterialAssetHandle{0, 1};
    lowerMaterial.materialKey = makeMaterialKey(
        lowerMaterial.materialHandle);
    lowerMaterial.viewDepth = 20.0f;
    lowerMaterial.candidateIndex = 1;

    MaterialRenderState doubleSidedState = makeOpaqueMaterialState();
    doubleSidedState.doubleSided = true;
    RenderItem alternatePipeline = regularNear;
    alternatePipeline.pipelineKey = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        doubleSidedState);
    alternatePipeline.materialHandle = MaterialAssetHandle{9, 1};
    alternatePipeline.materialKey = makeMaterialKey(
        alternatePipeline.materialHandle);
    alternatePipeline.viewDepth = 15.0f;
    alternatePipeline.candidateIndex = 5;

    RenderItem alphaClip = regularNear;
    alphaClip.queue = RenderQueue::AlphaClip;
    MaterialRenderState alphaClipState = makeOpaqueMaterialState();
    alphaClipState.alphaClipEnabled = true;
    alphaClip.pipelineKey = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        alphaClipState);
    alphaClip.viewDepth = 1.0f;
    alphaClip.candidateIndex = 4;

    std::vector<RenderItem> opaque = {
        alphaClip,
        regularFar,
        regularNear,
        alternatePipeline,
        lowerMaterial
    };
    std::sort(
        opaque.begin(),
        opaque.end(),
        OpaqueRenderItemComparator{});
    if (opaque[0].candidateIndex != 2 ||
        opaque[1].candidateIndex != 5 ||
        opaque[2].candidateIndex != 1 ||
        opaque[3].candidateIndex != 3 ||
        opaque[4].candidateIndex != 4)
    {
        throw std::runtime_error(
            "opaque RenderItem comparator produced an invalid order");
    }

    RenderItem transparentNear = regularNear;
    transparentNear.queue = RenderQueue::Transparent;
    transparentNear.pipelineKey = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        makeTransparentMaterialState());
    transparentNear.candidateIndex = 7;

    RenderItem transparentFarHighMaterial = transparentNear;
    transparentFarHighMaterial.viewDepth = 10.0f;
    transparentFarHighMaterial.candidateIndex = 6;

    RenderItem transparentFarLowMaterial =
        transparentFarHighMaterial;
    transparentFarLowMaterial.materialHandle = MaterialAssetHandle{0, 1};
    transparentFarLowMaterial.materialKey = makeMaterialKey(
        transparentFarLowMaterial.materialHandle);
    transparentFarLowMaterial.candidateIndex = 5;

    std::vector<RenderItem> transparent = {
        transparentNear,
        transparentFarHighMaterial,
        transparentFarLowMaterial
    };
    std::sort(
        transparent.begin(),
        transparent.end(),
        TransparentRenderItemComparator{});
    if (transparent[0].candidateIndex != 5 ||
        transparent[1].candidateIndex != 6 ||
        transparent[2].candidateIndex != 7)
    {
        throw std::runtime_error(
            "transparent RenderItem comparator produced an invalid order");
    }
}

void validateCullingSystem()
{
    Camera testCamera;
    Camera::Config cameraConfig{};
    cameraConfig.fov = 90.0f;
    cameraConfig.aspectRatio = 1.0f;
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 10.0f;
    testCamera.setConfig(cameraConfig);

    RenderView view = testCamera.makeRenderView();
    view.cullingMask = RenderLayer::World;
    view.cullingFlags = CullingFlags::All;

    std::vector<RenderCandidate> candidates(4);
    candidates[0].worldBounds = {
        {-0.1f, -0.1f, -1.1f},
        { 0.1f,  0.1f, -0.9f}
    };
    candidates[1].worldBounds = {
        {-0.1f, -0.1f, 0.9f},
        { 0.1f,  0.1f, 1.1f}
    };
    candidates[2].worldBounds = candidates[1].worldBounds;
    candidates[2].boundsCullingMode = BoundsCullingMode::Disabled;
    candidates[3].worldBounds = candidates[0].worldBounds;
    candidates[3].layerMask = RenderLayer::Editor;

    const CullingResults culled =
        CullingSystem{}.cull(candidates, view);
    if (culled.inputCount != 4 || culled.visibleCount() != 2 ||
        culled.visibleCandidateIndices[0] != 0 ||
        culled.visibleCandidateIndices[1] != 2 ||
        culled.layerCulledCount != 1 ||
        culled.frustumCulledCount != 1 ||
        culled.boundsCullingDisabledCount != 1)
    {
        throw std::runtime_error(
            "culling system produced unexpected visibility results");
    }

    view.cullingFlags = CullingFlags::None;
    const CullingResults unculled =
        CullingSystem{}.cull(candidates, view);
    if (unculled.visibleCount() != candidates.size() ||
        unculled.layerCulledCount != 0 ||
        unculled.frustumCulledCount != 0)
    {
        throw std::runtime_error(
            "disabled culling did not preserve every candidate");
    }

    Camera otherCamera;
    if (otherCamera.viewId() == testCamera.viewId())
    {
        throw std::runtime_error("different cameras reused a RenderViewId");
    }
    const uint64_t previousRevision = testCamera.gpuDataRevision();
    testCamera.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    const RenderView changedView = testCamera.makeRenderView();
    if (changedView.id != view.id ||
        changedView.gpuDataRevision == previousRevision)
    {
        throw std::runtime_error(
            "camera RenderView identity or GPU revision is invalid");
    }
}

} // namespace

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
    validateRenderKeys();
    validateRenderItemComparators();
    validateCullingSystem();
    createDemoAssets();

    const std::vector<RenderCandidate> candidates =
        SceneRenderExtractor{}.extract(scene, assetManager);
    if (candidates.empty())
    {
        throw std::runtime_error(
            "CPU-only scene extraction produced no render candidates");
    }
    for (const RenderCandidate& candidate : candidates)
    {
        if (!assetManager.contains(candidate.mesh) ||
            !assetManager.contains(candidate.material) ||
            !candidate.worldBounds.valid())
        {
            throw std::runtime_error(
                "CPU-only scene extraction produced an invalid candidate");
        }
    }

    const ModelAsset& model = assetManager.model(demoModelAsset);
    for (const ModelNode& node : model.nodes())
    {
        for (MeshAssetHandle meshHandle : node.meshes)
        {
            if (!assetManager.mesh(meshHandle).localBounds().valid())
            {
                throw std::runtime_error(
                    "imported mesh produced invalid local bounds");
            }
        }
    }
}

void App::runRenderTest()
{
    initWindow(false);
    initVulkan();
    for (uint32_t frame = 0; frame < 3; ++frame)
    {
        window.pollEvents();
        const RenderFrame renderFrame = makeRenderFrame();
        if (renderFrame.renderList.empty())
        {
            throw std::runtime_error(
                "scene extraction produced no render objects");
        }

        const auto validateItems = [&](const auto& items)
        {
            for (const RenderItem& item : items)
            {
                if (item.objectIndex >=
                        renderFrame.renderList.objectData.size() ||
                    !std::isfinite(item.viewDepth) ||
                    !item.materialKey ||
                    !item.pipelineKey)
                {
                    throw std::runtime_error(
                        "render-list construction produced an invalid item");
                }
            }
        };
        validateItems(renderFrame.renderList.opaque);
        validateItems(renderFrame.renderList.transparent);

        if (frame == 0)
        {
            std::clog
                << "[Render] Visible submesh draws="
                << renderFrame.renderList.size()
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

    RenderView view = camera.makeRenderView();

    std::vector<RenderCandidate> candidates =
        SceneRenderExtractor{}.extract(
            scene,
            assetManager);
    const CullingResults cullingResults =
        CullingSystem{}.cull(candidates, view);

    RenderFrame frame{};
    frame.renderList = RenderListBuilder{}.build(
        candidates,
        cullingResults,
        view,
        assetManager,
        renderAssets);
    frame.view = std::move(view);
    return frame;
}

} // namespace VkRenderer
