#include "Test/AppSmokeTests.h"

#include "ApplicationGui.h"
#include "ApplicationGuiRenderBridge.h"
#include "Asset/AssetId.h"
#include "Content/DemoContent.h"
#include "Import/KtxTextureImporter.h"
#include "Import/KtxTextureCooker.h"
#include "Render/CullingSystem.h"
#include "Render/MaterialKey.h"
#include "Render/PipelineVariantKey.h"
#include "Render/RenderFrameBuilder.h"
#include "Render/RenderItemComparator.h"
#include "Render/SceneRenderExtractor.h"
#include "RuntimeGui.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/GpuTexture.h"
#include "Vulkan/UploadContext.h"
#include "Vulkan/VulkanDrawListCompiler.h"

#include <imgui.h>
#include <ktx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
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
    regularNear.material = MaterialAssetHandle{1, 1};
    regularNear.materialKey = makeMaterialKey(regularNear.material);
    regularNear.mesh = MeshAssetHandle{1, 1};
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
    lowerMaterial.material = MaterialAssetHandle{0, 1};
    lowerMaterial.materialKey = makeMaterialKey(
        lowerMaterial.material);
    lowerMaterial.viewDepth = 20.0f;
    lowerMaterial.candidateIndex = 1;

    MaterialRenderState doubleSidedState = makeOpaqueMaterialState();
    doubleSidedState.doubleSided = true;
    RenderItem alternatePipeline = regularNear;
    alternatePipeline.pipelineKey = makePipelineVariantKey(
        MaterialTemplateAssetHandle{1, 1},
        doubleSidedState);
    alternatePipeline.material = MaterialAssetHandle{9, 1};
    alternatePipeline.materialKey = makeMaterialKey(
        alternatePipeline.material);
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
    transparentFarLowMaterial.material = MaterialAssetHandle{0, 1};
    transparentFarLowMaterial.materialKey = makeMaterialKey(
        transparentFarLowMaterial.material);
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
    const TextureAsset& flatNormal = assets.texture(
        content.defaultNormalTexture);
    if (flatNormal.colorSpace() != TextureColorSpace::Linear ||
        flatNormal.payload().size() != 4 ||
        flatNormal.payload()[0] != std::byte{0x80} ||
        flatNormal.payload()[1] != std::byte{0x80} ||
        flatNormal.payload()[2] != std::byte{0xff})
    {
        throw std::runtime_error(
            "default normal texture is not a linear flat tangent-space normal");
    }

    bool foundImportedPbrMaterial = false;
    const ModelAsset& model = assets.model(content.model);
    for (const ModelNode& node : model.nodes())
    {
        for (MeshAssetHandle meshHandle : node.meshes)
        {
            for (const SubmeshData& submesh :
                 assets.mesh(meshHandle).submeshes())
            {
                const MaterialAsset& material =
                    assets.material(submesh.material);
                if (material.textures().size() != 5)
                {
                    continue;
                }
                foundImportedPbrMaterial = true;
                const auto colorSpace = [&](uint32_t slot)
                {
                    return assets.texture(
                        material.textures()[slot]).colorSpace();
                };
                if (colorSpace(0) != TextureColorSpace::Srgb ||
                    colorSpace(1) != TextureColorSpace::Linear ||
                    colorSpace(2) != TextureColorSpace::Linear ||
                    colorSpace(3) != TextureColorSpace::Linear ||
                    colorSpace(4) != TextureColorSpace::Srgb)
                {
                    throw std::runtime_error(
                        "glTF material textures use incorrect semantic color spaces");
                }
            }
        }
    }
    if (!foundImportedPbrMaterial)
    {
        throw std::runtime_error(
            "demo model contains no imported PBR material to validate");
    }

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
        hasValidationIssue(
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
        {"metallicRoughnessTexture", content.defaultDataTexture},
        {"normalTexture", content.defaultNormalTexture},
        {"occlusionTexture", content.defaultDataTexture},
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
                !item.mesh ||
                !item.material ||
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

void validateVulkanDrawListCompilation(
    const RenderFrame& renderFrame,
    const RenderAssetCache& renderAssets)
{
    const VulkanDrawList resolved = VulkanDrawListCompiler{}.compile(
        renderFrame.renderList,
        renderAssets);
    if (resolved.size() != renderFrame.renderList.size())
    {
        throw std::runtime_error(
            "Vulkan draw-list compilation changed the draw count");
    }

    RenderList staleList = renderFrame.renderList;
    RenderItem* staleItem = !staleList.opaque.empty()
        ? &staleList.opaque.front()
        : &staleList.transparent.front();
    ++staleItem->mesh.generation;

    bool staleHandleRejected = false;
    try
    {
        static_cast<void>(VulkanDrawListCompiler{}.compile(
            staleList,
            renderAssets));
    }
    catch (const std::invalid_argument&)
    {
        staleHandleRejected = true;
    }
    if (!staleHandleRejected)
    {
        throw std::runtime_error(
            "Vulkan draw-list compilation accepted a stale GPU handle");
    }
}

void validateTextureFormats()
{
    const TextureFormatInfo rgba8 =
        textureFormatInfo(TextureFormat::RGBA8UNorm);
    const TextureFormatInfo bc1 =
        textureFormatInfo(TextureFormat::BC1RGBAUNorm);
    const TextureFormatInfo bc7 =
        textureFormatInfo(TextureFormat::BC7UNorm);
    if (rgba8.compressed || rgba8.bytesPerBlock != 4 ||
        !rgba8.supportsSrgb ||
        !bc1.compressed || bc1.blockWidth != 4 ||
        bc1.blockHeight != 4 || bc1.bytesPerBlock != 8 ||
        !bc1.supportsSrgb ||
        !bc7.compressed || bc7.bytesPerBlock != 16 ||
        !bc7.supportsSrgb ||
        textureMipByteSize(TextureFormat::RGBA8UNorm, 4, 4) != 64 ||
        textureMipByteSize(TextureFormat::BC1RGBAUNorm, 7, 5) != 32 ||
        textureMipByteSize(TextureFormat::BC7UNorm, 2, 2) != 16)
    {
        throw std::runtime_error("texture format layout validation failed");
    }
}

class TemporaryDirectory final
{
public:
    explicit TemporaryDirectory(std::filesystem::path path)
        : path_(std::move(path))
    {
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void validateKtxFileCookAndImport()
{
    const auto suffix = std::chrono::high_resolution_clock::now()
        .time_since_epoch().count();
    const TemporaryDirectory temporary(
        std::filesystem::temp_directory_path() /
        ("learnvulkan_toktx_" + std::to_string(suffix)));
    const std::filesystem::path inputPath =
        temporary.path() / "source.ppm";
    const std::filesystem::path outputPath =
        temporary.path() / "cooked.ktx2";

    {
        std::ofstream input(inputPath, std::ios::binary);
        constexpr std::array<uint8_t, 48> pixels{
            255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255,
            0, 255, 0, 0, 0, 255, 255, 255, 255, 255, 0, 0,
            0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 0,
            255, 255, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255};
        input << "P6\n4 4\n255\n";
        input.write(
            reinterpret_cast<const char*>(pixels.data()),
            static_cast<std::streamsize>(pixels.size()));
        if (!input)
        {
            throw std::runtime_error(
                "failed to write the toktx smoke-test source image");
        }
    }

    KtxTextureCooker::Request request{};
    request.inputPath = inputPath;
    request.outputPath = outputPath;
    request.colorSpace = TextureColorSpace::Srgb;
    request.generateMipmaps = true;
    request.mipFilter = TextureMipFilter::Mitchell;
    request.mipEdgeMode = TextureMipEdgeMode::Clamp;
    request.basis.encoding = KtxPayloadEncoding::Uastc;
    request.basis.uastcQualityLevel = 0;
    request.basis.threadCount = 1;
    request.zstdLevel = 1;

    KtxTextureImporter::CreateInfo importInfo{};
    importInfo.name = "toktx file pipeline smoke texture";
    importInfo.transcodeFormat = TextureFormat::BC7UNorm;
    const TextureAsset::CreateInfo textureInfo =
        KtxTextureCooker{}.cookAndImport(request, importInfo);
    if (!std::filesystem::is_regular_file(outputPath) ||
        textureInfo.format != TextureFormat::BC7UNorm ||
        textureInfo.colorSpace != TextureColorSpace::Srgb ||
        textureInfo.mipLevels.size() != 3 ||
        textureInfo.payload.size() != 48)
    {
        throw std::runtime_error(
            "toktx local-file cook/import pipeline produced invalid texture data");
    }
}

[[noreturn]] void throwKtxTestError(
    const char* operation,
    KTX_error_code error)
{
    throw std::runtime_error(
        std::string(operation) + ": " + ktxErrorString(error));
}

std::vector<uint8_t> makeKtx2Fixture(bool basisEncoded)
{
    ktxTextureCreateInfo createInfo{};
    createInfo.vkFormat = basisEncoded
        ? VK_FORMAT_R8G8B8A8_UNORM
        : VK_FORMAT_BC7_UNORM_BLOCK;
    createInfo.baseWidth = 4;
    createInfo.baseHeight = 4;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = 3;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;

    ktxTexture2* rawTexture = nullptr;
    KTX_error_code error = ktxTexture2_Create(
        &createInfo,
        KTX_TEXTURE_CREATE_ALLOC_STORAGE,
        &rawTexture);
    if (error != KTX_SUCCESS)
    {
        throwKtxTestError("failed to create KTX2 test texture", error);
    }
    using TexturePointer =
        std::unique_ptr<ktxTexture2, decltype(&ktxTexture2_Destroy)>;
    TexturePointer texture(rawTexture, &ktxTexture2_Destroy);

    for (uint32_t mipLevel = 0; mipLevel < createInfo.numLevels; ++mipLevel)
    {
        const uint32_t width = std::max(1u, createInfo.baseWidth >> mipLevel);
        const uint32_t height = std::max(1u, createInfo.baseHeight >> mipLevel);
        const TextureFormat format = basisEncoded
            ? TextureFormat::RGBA8UNorm
            : TextureFormat::BC7UNorm;
        std::vector<uint8_t> levelData(
            textureMipByteSize(format, width, height),
            static_cast<uint8_t>(0x40 + mipLevel * 0x20));
        error = ktxTexture_SetImageFromMemory(
            ktxTexture(texture.get()),
            mipLevel,
            0,
            0,
            levelData.data(),
            levelData.size());
        if (error != KTX_SUCCESS)
        {
            throwKtxTestError("failed to populate KTX2 test mip", error);
        }
    }

    if (basisEncoded)
    {
        error = ktxTexture2_CompressBasis(texture.get(), 128);
        if (error != KTX_SUCCESS)
        {
            throwKtxTestError("failed to Basis-encode KTX2 test texture", error);
        }
    }

    ktx_uint8_t* encodedData = nullptr;
    ktx_size_t encodedSize = 0;
    error = ktxTexture2_WriteToMemory(
        texture.get(),
        &encodedData,
        &encodedSize);
    if (error != KTX_SUCCESS)
    {
        throwKtxTestError("failed to serialize KTX2 test texture", error);
    }
    using EncodedPointer =
        std::unique_ptr<ktx_uint8_t, decltype(&std::free)>;
    EncodedPointer encoded(encodedData, &std::free);
    return std::vector<uint8_t>(encoded.get(), encoded.get() + encodedSize);
}

void validateKtxTextureImportAndUpload(const Device& device)
{
    const KtxTextureImporter importer;
    KtxTextureImporter::CreateInfo importInfo{};
    importInfo.name = "Direct BC7 KTX2 smoke texture";

    const std::vector<uint8_t> directBytes = makeKtx2Fixture(false);
    TextureAsset::CreateInfo directInfo = importer.importMemory(
        directBytes.data(),
        directBytes.size(),
        importInfo);
    if (directInfo.format != TextureFormat::BC7UNorm ||
        directInfo.colorSpace != TextureColorSpace::Linear ||
        directInfo.payload.size() != 48 ||
        directInfo.mipLevels.size() != 3)
    {
        throw std::runtime_error(
            "direct BC7 KTX2 import produced invalid asset metadata");
    }

    importInfo.name = "Basis-to-BC7 KTX2 smoke texture";
    importInfo.transcodeFormat = TextureFormat::BC7UNorm;
    const std::vector<uint8_t> basisBytes = makeKtx2Fixture(true);
    TextureAsset::CreateInfo textureInfo = importer.importMemory(
        basisBytes.data(),
        basisBytes.size(),
        importInfo);
    if (textureInfo.format != TextureFormat::BC7UNorm ||
        textureInfo.colorSpace != TextureColorSpace::Linear ||
        textureInfo.payload.size() != 48 ||
        textureInfo.mipLevels.size() != 3)
    {
        throw std::runtime_error(
            "Basis-to-BC7 KTX2 transcode produced invalid asset metadata");
    }

    CommandPool commandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    UploadContext uploadContext(device, commandPool);

    const TextureAsset texture(std::move(textureInfo));
    GpuTexture::CreateInfo gpuTextureInfo{};
    gpuTextureInfo.asset = &texture;
    gpuTextureInfo.viewRange.baseMipLevel = 1;
    gpuTextureInfo.viewRange.levelCount = 2;
    const GpuTexture gpuTexture(device, uploadContext, gpuTextureInfo);
    if (!gpuTexture || gpuTexture.format() != VK_FORMAT_BC7_UNORM_BLOCK)
    {
        throw std::runtime_error(
            "transcoded BC7 KTX2 upload produced an invalid GPU texture");
    }
}

TextureAsset::CreateInfo cloneTextureCreateInfo(
    const TextureAsset& source,
    std::string name)
{
    TextureAsset::CreateInfo result{};
    result.name = std::move(name);
    result.width = source.width();
    result.height = source.height();
    result.format = source.format();
    result.colorSpace = source.colorSpace();
    result.sampler = source.sampler();
    result.payload = source.payload();
    result.mipLevels = source.mipLevels();
    return result;
}

void validateStableTextureAssetReplacement()
{
    AssetManager assets;
    TextureAsset::CreateInfo original{};
    original.name = "Original";
    original.width = 1;
    original.height = 1;
    original.format = TextureFormat::RGBA8UNorm;
    original.payload = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0xff}};
    const TextureAssetHandle handle =
        assets.createTexture(std::move(original));

    TextureAsset::CreateInfo replacement = cloneTextureCreateInfo(
        assets.texture(handle),
        "Replacement");
    replacement.payload[0] = std::byte{0x80};
    TextureAsset previous = assets.replaceTexture(
        handle,
        TextureAsset(std::move(replacement)));
    if (!assets.contains(handle) ||
        assets.texture(handle).name() != "Replacement" ||
        previous.name() != "Original" ||
        assets.texture(handle).payload()[0] != std::byte{0x80})
    {
        throw std::runtime_error(
            "TextureAsset replacement did not preserve its handle and content boundary");
    }
}

void validateGpuTextureReplacement(
    const Device& device,
    AssetManager& assets,
    RenderAssetCache& renderAssets,
    TextureAssetHandle handle)
{
    const GpuTexture* originalGpu = renderAssets.tryTexture(handle);
    if (originalGpu == nullptr)
    {
        throw std::runtime_error(
            "GPU texture replacement test requires a cached texture");
    }
    const VkImageView originalView = originalGpu->view();

    TextureAsset replacementAsset(cloneTextureCreateInfo(
        assets.texture(handle),
        "GPU replacement texture"));
    CommandPool commandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    UploadContext uploadContext(device, commandPool);
    GpuTexture staged = renderAssets.stageTextureReplacement(
        device,
        uploadContext,
        replacementAsset);
    GpuTexture previousGpu = renderAssets.commitTextureReplacement(
        device,
        assets,
        handle,
        std::move(staged));
    TextureAsset previousAsset = assets.replaceTexture(
        handle,
        std::move(replacementAsset));

    if (!previousGpu || !previousAsset || !assets.contains(handle) ||
        renderAssets.tryTexture(handle) == nullptr ||
        renderAssets.texture(handle).view() == originalView)
    {
        throw std::runtime_error(
            "GPU texture replacement did not preserve the cache handle or replace its image view");
    }
}

} // namespace

void AppSmokeTests::runAssetImportTest()
{
    App app;

    validateAssetId();
    validateTextureFormats();
    validateStableTextureAssetReplacement();
    validateKtxFileCookAndImport();
    validateRenderKeys();
    validateRenderItemComparators();
    validateCullingSystem();
    const App::RunConfig config{};
    app.demoContent = DemoContentLoader::load(
        app.assetManager,
        app.scene,
        config.demoContent,
        &app.textureImports);
    if (app.textureImports.size() != 5)
    {
        throw std::runtime_error(
            "extracted glTF textures did not register reimport provenance");
    }
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

    Camera frontendCamera;
    frontendCamera.setPosition(glm::vec3(0.0f, 1.0f, 0.5f));
    frontendCamera.setRotation(glm::vec3(-60.0f, 0.0f, 0.0f));
    frontendCamera.setAspect(16.0f / 9.0f);
    frontendCamera.Update();
    const RenderFrame frontendFrame = buildRenderFrame(
        app.scene,
        app.assetManager,
        frontendCamera.makeRenderView());
    validateRenderFrame(frontendFrame);

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
    validateKtxTextureImportAndUpload(app.vulkanContext.device());
    app.initImGui(config);
    app.guiRenderBridge.attach(app.renderer, app.renderAssets);
    ApplicationGuiContext guiContext{
        app.assetManager,
        app.scene,
        app.guiRenderBridge
    };
    gui.attach(guiContext);
    TextureAssetHandle editorPreviewTexture =
        app.demoContent.defaultTexture;
    if (app.renderAssets.tryTexture(editorPreviewTexture) == nullptr)
    {
        const std::vector<RenderCandidate> previewCandidates =
            SceneRenderExtractor{}.extract(app.scene, app.assetManager);
        for (const RenderCandidate& candidate : previewCandidates)
        {
            const MaterialAsset& material =
                app.assetManager.material(candidate.material);
            const auto texture = std::find_if(
                material.textures().begin(),
                material.textures().end(),
                [&](TextureAssetHandle handle)
                {
                    return app.renderAssets.tryTexture(handle) != nullptr;
                });
            if (texture != material.textures().end())
            {
                editorPreviewTexture = *texture;
                break;
            }
        }
    }
    validateGpuTextureReplacement(
        app.vulkanContext.device(),
        app.assetManager,
        app.renderAssets,
        editorPreviewTexture);
    if (config.outputMode == VulkanRenderer::OutputMode::Editor)
    {
        const TextureImportRecord* before =
            app.textureImports.find(editorPreviewTexture);
        if (before == nullptr)
        {
            throw std::runtime_error(
                "Editor texture reimport test requires source provenance");
        }
        const uint64_t previousRevision = before->revision;
        TextureImportSettings asynchronousSettings = before->settings;
        asynchronousSettings.basis.encoding = KtxPayloadEncoding::Uastc;
        asynchronousSettings.basis.uastcQualityLevel = 0;
        asynchronousSettings.zstdLevel = 0;
        asynchronousSettings.transcodeFormat = TextureFormat::BC7UNorm;
        app.pendingTextureReimport_ = TextureReimportRequest{
            editorPreviewTexture,
            std::move(asynchronousSettings)};
        app.processPendingTextureReimport();

        const TextureImportRecord* started =
            app.textureImports.find(editorPreviewTexture);
        if (started == nullptr || !started->reimporting ||
            started->revision != previousRevision)
        {
            throw std::runtime_error(
                "Editor texture reimport did not enter its asynchronous cooking state");
        }

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (std::chrono::steady_clock::now() < deadline)
        {
            app.processPendingTextureReimport();
            const TextureImportRecord* current =
                app.textureImports.find(editorPreviewTexture);
            if (current == nullptr || !current->reimporting)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const TextureImportRecord* after =
            app.textureImports.find(editorPreviewTexture);
        if (after == nullptr || after->revision != previousRevision + 1 ||
            !after->lastError.empty() ||
            !std::filesystem::is_regular_file(after->cookedPath))
        {
            throw std::runtime_error(
                "Editor texture reimport did not commit KTX2, CPU asset, and GPU asset together");
        }
    }
    try
    {
        if (config.outputMode == VulkanRenderer::OutputMode::Editor)
        {
            const ApplicationGuiTexture smokeTexturePreview =
                app.guiRenderBridge.preview(
                editorPreviewTexture);
            if (!smokeTexturePreview)
            {
                throw std::runtime_error(
                    "Editor texture preview registration failed");
            }
        }

        for (uint32_t frame = 0; frame < 3; ++frame)
        {
            app.window.pollEvents();
            app.imguiLayer.beginFrame();
            app.drawGui(gui);
            if (config.outputMode == VulkanRenderer::OutputMode::Editor)
            {
                const ApplicationGuiTexture smokeTexturePreview =
                    app.guiRenderBridge.preview(
                        editorPreviewTexture);
                ImGui::Begin("Texture Preview Smoke Test");
                ImGui::Image(
                    ImTextureRef(static_cast<ImTextureID>(
                        smokeTexturePreview.textureId)),
                    ImVec2(16.0f, 16.0f));
                ImGui::End();
            }
            ImDrawData* uiDrawData = app.imguiLayer.endFrame();

            const RenderFrame renderFrame = app.makeRenderFrame();
            validateRenderFrame(renderFrame);

            if (frame == 0)
            {
                validateVulkanDrawListCompilation(
                    renderFrame,
                    app.renderAssets);
                std::clog
                    << "[Render] Visible submesh draws="
                    << renderFrame.renderList.size()
                    << '\n';
            }
            if (app.renderer.render(
                    renderFrame,
                    app.renderAssets,
                    uiDrawData) ==
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
