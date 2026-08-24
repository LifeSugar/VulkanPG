#include "Test/AppSmokeTests.h"

#include "ApplicationGui.h"
#include "Asset/AssetId.h"
#include "Content/DemoContent.h"
#include "Render/CullingSystem.h"
#include "Render/MaterialKey.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderFrameBuilder.h"
#include "Render/RenderItemComparator.h"
#include "Render/SceneRenderExtractor.h"
#include "RuntimeGui.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace Test
{
namespace
{

[[nodiscard]] bool hasValidationIssue(
    const ValidationReport& report,
    const std::string& code,
    ValidationSeverity severity)
{
    return std::any_of(
        report.issues().begin(),
        report.issues().end(),
        [&](const ValidationIssue& issue)
        {
            return issue.code == code && issue.severity == severity;
        });
}

void validateAssetId()
{
    constexpr std::string_view canonical =
        "00112233-4455-6677-8899-aabbccddeeff";
    const std::optional<AssetId> parsed = AssetId::parse(canonical);
    if (!parsed ||
        parsed->high != UINT64_C(0x0011223344556677) ||
        parsed->low != UINT64_C(0x8899aabbccddeeff) ||
        parsed->toString() != canonical)
    {
        throw std::runtime_error(
            "AssetId canonical serialization round trip failed");
    }

    const std::optional<AssetId> uppercase = AssetId::parse(
        "00112233-4455-6677-8899-AABBCCDDEEFF");
    if (!uppercase || *uppercase != *parsed ||
        AssetId::parse("00112233445566778899aabbccddeeff") ||
        AssetId::parse("00112233-4455-6677-8899-aabbccddeefg"))
    {
        throw std::runtime_error("AssetId parsing validation failed");
    }

    const std::optional<AssetId> nil = AssetId::parse(
        "00000000-0000-0000-0000-000000000000");
    if (!nil || nil->valid())
    {
        throw std::runtime_error("AssetId nil representation is invalid");
    }

    std::unordered_set<AssetId> generatedIds;
    for (uint32_t index = 0; index < 64; ++index)
    {
        const AssetId generated = AssetId::generate();
        const std::optional<AssetId> roundTrip =
            AssetId::parse(generated.toString());
        if (!generated || !roundTrip || *roundTrip != generated ||
            (generated.high & UINT64_C(0x000000000000f000)) !=
                UINT64_C(0x0000000000004000) ||
            (generated.low & UINT64_C(0xc000000000000000)) !=
                UINT64_C(0x8000000000000000) ||
            !generatedIds.insert(generated).second)
        {
            throw std::runtime_error(
                "generated AssetId is invalid or duplicated");
        }
    }
}

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

void validateOfflineMaterialAssets(
    AssetManager& assets,
    const DemoContent& content)
{
    const ShaderAsset& fragmentShader = assets.shader(
        content.pbrFragmentShader);
    const auto materialBlock = std::find_if(
        fragmentShader.interface().parameterBlocks.begin(),
        fragmentShader.interface().parameterBlocks.end(),
        [](const ShaderParameterBlockDesc& block)
        {
            return block.set == 1 && block.binding == 0;
        });
    if (fragmentShader.interface().signature == 0 ||
        materialBlock == fragmentShader.interface().parameterBlocks.end() ||
        materialBlock->byteSize != 36 ||
        materialBlock->members.size() != 4)
    {
        throw std::runtime_error(
            "CPU-only SPIR-V reflection produced an invalid material interface");
    }

    const MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(content.materialTemplate);
    MaterialTemplateAsset::CreateInfo templateInfo{};
    templateInfo.name = materialTemplate.name();
    templateInfo.shaders = materialTemplate.shaders();
    templateInfo.parameterBlock = materialTemplate.parameterBlock();
    templateInfo.parameterDataSize = materialTemplate.parameterDataSize();
    templateInfo.parameters = materialTemplate.parameters();
    templateInfo.textureSlots = materialTemplate.textureSlots();

    const ValidationReport currentTemplateReport =
        assets.validateMaterialTemplate(templateInfo);
    if (!currentTemplateReport.valid() ||
        !hasValidationIssue(
            currentTemplateReport,
            "Template.InactiveImageBinding",
            ValidationSeverity::Warning) ||
        !assets.isMaterialTemplateCurrent(content.materialTemplate))
    {
        throw std::runtime_error(
            "valid material template did not pass offline validation");
    }

    MaterialTemplateAsset::CreateInfo invalidTemplate = templateInfo;
    invalidTemplate.parameters.back().byteOffset = 36;
    const ValidationReport invalidTemplateReport =
        assets.validateMaterialTemplate(invalidTemplate);
    if (invalidTemplateReport.valid() ||
        !hasValidationIssue(
            invalidTemplateReport,
            "Template.ParameterOffsetMismatch",
            ValidationSeverity::Error))
    {
        throw std::runtime_error(
            "invalid material template passed shader-interface validation");
    }

    MaterialAsset::CreateInfo invalidMaterial{};
    invalidMaterial.name = "Invalid Offline Material";
    invalidMaterial.materialTemplate = content.materialTemplate;
    invalidMaterial.parameters = {
        {"baseColorFactor", glm::vec4(1.0f)},
        {"emissiveFactor", glm::vec3(0.0f)},
        {"metallicFactor", 0.0f},
        {"roughnessFactor", glm::vec4(1.0f)}
    };
    invalidMaterial.textures = {
        {"baseColorTexture", content.defaultTexture},
        {"metallicRoughnessTexture", content.defaultTexture},
        {"normalTexture", content.defaultTexture},
        {"occlusionTexture", content.defaultTexture},
        {"emissiveTexture", content.defaultTexture}
    };
    const ValidationReport invalidMaterialReport =
        assets.validateMaterial(invalidMaterial);
    if (invalidMaterialReport.valid() ||
        !hasValidationIssue(
            invalidMaterialReport,
            "Material.ParameterTypeMismatch",
            ValidationSeverity::Error))
    {
        throw std::runtime_error(
            "invalid material instance passed template validation");
    }

    bool creationRejected = false;
    try
    {
        static_cast<void>(assets.createMaterial(
            std::move(invalidMaterial)));
    }
    catch (const AssetValidationError& error)
    {
        creationRejected = hasValidationIssue(
            error.report(),
            "Material.ParameterTypeMismatch",
            ValidationSeverity::Error);
    }
    if (!creationRejected)
    {
        throw std::runtime_error(
            "AssetManager accepted an invalid material instance");
    }
}


void validateRenderFrame(const RenderFrame& renderFrame)
{
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
}

} // namespace

void AppSmokeTests::runAssetImportTest()
{
    App app;

    validateAssetId();
    validateRenderKeys();
    validateRenderItemComparators();
    validateCullingSystem();
    app.demoContent = DemoContentLoader::load(app.assetManager, app.scene);
    validateOfflineMaterialAssets(app.assetManager, app.demoContent);

    const std::vector<RenderCandidate> candidates =
        SceneRenderExtractor{}.extract(app.scene, app.assetManager);
    if (candidates.empty())
    {
        throw std::runtime_error(
            "CPU-only scene extraction produced no render candidates");
    }
    for (const RenderCandidate& candidate : candidates)
    {
        if (!app.assetManager.contains(candidate.mesh) ||
            !app.assetManager.contains(candidate.material) ||
            !candidate.worldBounds.valid())
        {
            throw std::runtime_error(
                "CPU-only scene extraction produced an invalid candidate");
        }
    }

    const ModelAsset& model =
        app.assetManager.model(app.demoContent.model);
    for (const ModelNode& node : model.nodes())
    {
        for (MeshAssetHandle meshHandle : node.meshes)
        {
            if (!app.assetManager.mesh(meshHandle).localBounds().valid())
            {
                throw std::runtime_error(
                    "imported mesh produced invalid local bounds");
            }
        }
    }
}

void AppSmokeTests::runRenderTest()
{
    App app;
    RuntimeGui gui;
    runRenderTest(app, App::RunConfig{}, gui);
}

void AppSmokeTests::runRenderTest(
    App& app,
    const App::RunConfig& config,
    ApplicationGui& gui)
{
    app.initWindow(config, false);
    app.initVulkan(config);
    app.initImGui(config);
    ApplicationGuiContext guiContext{
        app.assetManager,
        app.scene,
        app.renderer
    };
    gui.attach(guiContext);
    try
    {
        for (uint32_t frame = 0; frame < 3; ++frame)
        {
            app.window.pollEvents();
            app.imguiLayer.beginFrame();
            app.drawGui(gui);
            ImDrawData* uiDrawData = app.imguiLayer.endFrame();

            const RenderFrame renderFrame = app.makeRenderFrame();
            validateRenderFrame(renderFrame);

            if (frame == 0)
            {
                std::clog
                    << "[Render] Visible submesh draws="
                    << renderFrame.renderList.size()
                    << '\n';
            }
            if (app.renderer.render(renderFrame, uiDrawData) ==
                VulkanRenderer::RenderResult::NeedsResize)
            {
                throw std::runtime_error(
                    "hidden render test unexpectedly requires a resize");
            }

            if (frame == 0)
            {
                // Exercise render-pass replacement and GUI texture refresh.
                app.recreateSwapChain(gui);
            }
        }
    }
    catch (...)
    {
        app.renderer.waitIdle();
        gui.detach();
        app.cleanup();
        throw;
    }
    app.renderer.waitIdle();
    gui.detach();
    app.cleanup();
}

} // namespace Test
} // namespace VkRenderer
