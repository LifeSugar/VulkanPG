#include "AppSmokeTests.hpp"

#include "ApplicationGui.hpp"
#include "render/ApplicationGuiRenderBridge.hpp"
#include "asset/AssetId.hpp"
#include "asset/ShaderAsset.hpp"
#include "content/DemoContent.hpp"
#include "shader/SpirvReflection.hpp"
#include "shader/SpirvShaderImporter.hpp"
#include "texture/KtxTextureImporter.hpp"
#include "texture/KtxTextureCooker.hpp"
#include "render/CullingSystem.hpp"
#include "render/MaterialKey.hpp"
#include "render/PipelineVariantKey.hpp"
#include "render/RenderFrameBuilder.hpp"
#include "render/RenderItemComparator.hpp"
#include "render/SceneRenderExtractor.hpp"
#include "RuntimeGui.hpp"
#include "vulkan/CommandPool.hpp"
#include "vulkan/GpuTexture.hpp"
#include "vulkan/UploadContext.hpp"
#include "vulkan/VulkanDrawListCompiler.hpp"

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
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rubia
{
namespace test
{
namespace
{

[[nodiscard]] bool hasValidationIssue(
    const asset::ValidationReport& report,
    const std::string& code,
    asset::ValidationSeverity severity)
{
    return std::any_of(
        report.issues().begin(),
        report.issues().end(),
        [&](const asset::ValidationIssue& issue)
        {
            return issue.code == code && issue.severity == severity;
        });
}

void requireShaderTest(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error("shader asset test: " + message);
    }
}

[[nodiscard]] asset::ShaderStage shaderStageFromPath(
    const std::filesystem::path& path)
{
    const std::string stageExtension = path.stem().extension().string();
    if (stageExtension == ".vert" || stageExtension == ".vs")
    {
        return asset::ShaderStage::Vertex;
    }
    if (stageExtension == ".frag" || stageExtension == ".fs" ||
        stageExtension == ".ps")
    {
        return asset::ShaderStage::Fragment;
    }
    if (stageExtension == ".comp" || stageExtension == ".cs")
    {
        return asset::ShaderStage::Compute;
    }
    throw std::invalid_argument(
        "shader asset test: cannot infer stage from path; expected "
        "*.vert.spv, *.frag.spv, or *.comp.spv");
}

[[nodiscard]] const char* shaderStageName(asset::ShaderStage stage) noexcept
{
    switch (stage)
    {
    case asset::ShaderStage::Vertex: return "Vertex";
    case asset::ShaderStage::Fragment: return "Fragment";
    case asset::ShaderStage::Compute: return "Compute";
    }
    return "Unknown";
}

[[nodiscard]] const char* shaderDataTypeName(
    asset::ShaderDataType type) noexcept
{
    switch (type)
    {
    case asset::ShaderDataType::Unknown: return "Unknown";
    case asset::ShaderDataType::Float: return "Float";
    case asset::ShaderDataType::Float2: return "Float2";
    case asset::ShaderDataType::Float3: return "Float3";
    case asset::ShaderDataType::Float4: return "Float4";
    case asset::ShaderDataType::Matrix4: return "Matrix4";
    case asset::ShaderDataType::Int: return "Int";
    case asset::ShaderDataType::UInt: return "UInt";
    case asset::ShaderDataType::Bool: return "Bool";
    }
    return "Unknown";
}

[[nodiscard]] const char* descriptorTypeName(
    asset::ShaderDescriptorType type) noexcept
{
    switch (type)
    {
    case asset::ShaderDescriptorType::UniformBuffer: return "UniformBuffer";
    case asset::ShaderDescriptorType::StorageBuffer: return "StorageBuffer";
    case asset::ShaderDescriptorType::SampledImage: return "SampledImage";
    case asset::ShaderDescriptorType::Sampler: return "Sampler";
    case asset::ShaderDescriptorType::CombinedImageSampler:
        return "CombinedImageSampler";
    }
    return "Unknown";
}

[[nodiscard]] const char* resourceAccessName(
    asset::ShaderResourceAccess access) noexcept
{
    switch (access)
    {
    case asset::ShaderResourceAccess::ReadOnly: return "ReadOnly";
    case asset::ShaderResourceAccess::WriteOnly: return "WriteOnly";
    case asset::ShaderResourceAccess::ReadWrite: return "ReadWrite";
    }
    return "Unknown";
}

void printBufferLayout(
    const asset::ShaderBufferLayoutDesc& layout,
    const std::string& indent)
{
    std::cout
        << indent << "minimumByteSize: " << layout.minimumByteSize << '\n'
        << indent << "runtimeSized: "
        << (layout.runtimeSized() ? "true" : "false") << '\n'
        << indent << "members (" << layout.members.size() << "):\n";
    for (const asset::ShaderStructMemberDesc& member : layout.members)
    {
        std::cout
            << indent << "  - name=" << std::quoted(member.name)
            << " type=" << shaderDataTypeName(member.type)
            << " offset=" << member.offset
            << " size=" << member.size
            << " arrayCount=" << member.arrayCount
            << " runtimeArray="
            << (member.runtimeArray ? "true" : "false")
            << " arrayStride=" << member.arrayStride
            << " matrixStride=" << member.matrixStride
            << " rowMajor=" << (member.rowMajor ? "true" : "false")
            << '\n';
    }
}

void printShaderReflection(
    const std::filesystem::path& shaderPath,
    const asset::ShaderAsset& shader)
{
    const asset::ShaderInterface& interface = shader.interface();
    std::cout
        << "\n=== SPIR-V Reflection ===\n"
        << "path: " << shaderPath << '\n'
        << "name: " << std::quoted(shader.name()) << '\n'
        << "stage: " << shaderStageName(interface.stage) << '\n'
        << "entryPoint: " << std::quoted(interface.entryPoint) << '\n'
        << "spirvWords: " << shader.spirv().size() << '\n'
        << "interfaceHash: 0x" << std::hex << interface.interfaceHash
        << std::dec << "\n\n";

    const auto printStageVariables = [](const char* label, const auto& variables)
    {
        std::cout << label << " (" << variables.size() << "):\n";
        for (const asset::ShaderStageVariableDesc& variable : variables)
        {
            std::cout
                << "  - location=" << variable.location
                << " type=" << shaderDataTypeName(variable.type)
                << " name=" << std::quoted(variable.name) << '\n';
        }
        std::cout << '\n';
    };
    printStageVariables("inputs", interface.inputs);
    printStageVariables("outputs", interface.outputs);

    std::cout << "descriptors (" << interface.descriptorBindings.size()
              << "):\n";
    for (const asset::ShaderDescriptorBindingDesc& descriptor :
         interface.descriptorBindings)
    {
        std::cout
            << "  - set=" << descriptor.set
            << " binding=" << descriptor.binding
            << " type=" << descriptorTypeName(descriptor.type)
            << " access=" << resourceAccessName(descriptor.access)
            << " arrayCount=" << descriptor.arrayCount
            << " name=" << std::quoted(descriptor.name) << '\n';
        if (descriptor.bufferLayout)
        {
            printBufferLayout(*descriptor.bufferLayout, "      ");
        }
    }

    std::cout << "\npushConstants (" << interface.pushConstantBlocks.size()
              << "):\n";
    for (const asset::ShaderPushConstantBlockDesc& pushConstant :
         interface.pushConstantBlocks)
    {
        std::cout << "  - name=" << std::quoted(pushConstant.name) << '\n';
        printBufferLayout(pushConstant.layout, "      ");
    }
    std::cout << "=== End Reflection ===\n\n";
}

void validateBufferLayout(
    const asset::ShaderBufferLayoutDesc& layout,
    const std::string& owner)
{
    for (std::size_t index = 0; index < layout.members.size(); ++index)
    {
        const asset::ShaderStructMemberDesc& member = layout.members[index];
        if (member.runtimeArray)
        {
            requireShaderTest(
                index + 1 == layout.members.size() && member.size == 0 &&
                    member.arrayStride != 0 &&
                    member.offset <= layout.minimumByteSize,
                owner + " has an invalid runtime-array member");
            continue;
        }

        const uint64_t memberEnd =
            static_cast<uint64_t>(member.offset) + member.size;
        requireShaderTest(
            member.size != 0 && memberEnd <= layout.minimumByteSize,
            owner + " contains a member outside its declared byte size");
    }
}

void validateShaderAssetPipeline(const std::filesystem::path& shaderPath)
{
    requireShaderTest(
        shaderPath.is_absolute(),
        "shader path must be absolute: " + shaderPath.string());
    requireShaderTest(
        std::filesystem::is_regular_file(shaderPath),
        "shader path is not a regular file: " + shaderPath.string());

    const asset::ShaderStage expectedStage = shaderStageFromPath(shaderPath);
    asset::AssetManager assets;
    importer::shader::SpirvShaderImporter importer;

    importer::shader::SpirvShaderImporter::CreateInfo createInfo{};
    createInfo.assets = &assets;
    createInfo.path = shaderPath;
    createInfo.stage = expectedStage;
    createInfo.entryPoint = "main";

    const asset::ShaderAssetHandle handle = importer.import(createInfo);
    requireShaderTest(
        handle && assets.contains(handle),
        "importer returned an invalid ShaderAsset handle");

    const asset::ShaderAsset& shader = assets.shader(handle);
    const asset::ShaderInterface& interface = shader.interface();
    requireShaderTest(
        shader.stage() == expectedStage &&
            shader.entryPoint() == createInfo.entryPoint &&
            interface.stage == expectedStage &&
            interface.entryPoint == createInfo.entryPoint,
        "stage or entry-point metadata changed during import");
    requireShaderTest(
        !shader.spirv().empty() &&
            shader.spirv().front() == UINT32_C(0x07230203),
        "imported bytecode is not SPIR-V");
    requireShaderTest(
        interface.interfaceHash != 0,
        "reflected interface hash is zero");

    printShaderReflection(shaderPath, shader);

    const auto validateStageVariables = [](const auto& variables,
                                           const char* direction)
    {
        std::unordered_set<uint32_t> locations;
        for (const asset::ShaderStageVariableDesc& variable : variables)
        {
            requireShaderTest(
                locations.insert(variable.location).second,
                std::string("duplicate stage ") + direction +
                    " location " + std::to_string(variable.location));
        }
    };
    validateStageVariables(interface.inputs, "input");
    validateStageVariables(interface.outputs, "output");

    std::unordered_set<uint64_t> descriptorLocations;
    for (const asset::ShaderDescriptorBindingDesc& descriptor :
         interface.descriptorBindings)
    {
        const uint64_t location =
            (static_cast<uint64_t>(descriptor.set) << 32u) |
            descriptor.binding;
        requireShaderTest(
            descriptorLocations.insert(location).second,
            "duplicate descriptor set/binding");

        const bool isBuffer =
            descriptor.type == asset::ShaderDescriptorType::UniformBuffer ||
            descriptor.type == asset::ShaderDescriptorType::StorageBuffer;
        requireShaderTest(
            descriptor.bufferLayout.has_value() == isBuffer,
            "descriptor buffer-layout presence disagrees with its type");

        if (descriptor.type == asset::ShaderDescriptorType::UniformBuffer)
        {
            requireShaderTest(
                descriptor.access == asset::ShaderResourceAccess::ReadOnly &&
                    !descriptor.bufferLayout->runtimeSized(),
                "uniform buffer must be fixed-size and read-only");
        }
        if (descriptor.bufferLayout)
        {
            validateBufferLayout(
                *descriptor.bufferLayout,
                "descriptor set " + std::to_string(descriptor.set) +
                    " binding " + std::to_string(descriptor.binding));
        }
    }

    for (const asset::ShaderPushConstantBlockDesc& pushConstant :
         interface.pushConstantBlocks)
    {
        validateBufferLayout(
            pushConstant.layout,
            "push-constant block " + pushConstant.name);
    }

    const asset::ShaderInterface reflectedAgain =
        importer::shader::SpirvReflection::reflect(
            shader.spirv(),
            shader.stage(),
            shader.entryPoint());
    requireShaderTest(
        reflectedAgain.interfaceHash == interface.interfaceHash,
        "repeated reflection produced a different interface hash");

    bool missingEntryPointRejected = false;
    try
    {
        static_cast<void>(importer::shader::SpirvReflection::reflect(
            shader.spirv(),
            shader.stage(),
            "__rubia_missing_entry_point__"));
    }
    catch (const std::invalid_argument&)
    {
        missingEntryPointRejected = true;
    }
    requireShaderTest(
        missingEntryPointRejected,
        "reflection accepted a missing entry point");
}

void validateAssetId()
{
    constexpr std::string_view canonical =
        "00112233-4455-6677-8899-aabbccddeeff";
    const std::optional<asset::AssetId> parsed = asset::AssetId::parse(canonical);
    if (!parsed ||
        parsed->high != UINT64_C(0x0011223344556677) ||
        parsed->low != UINT64_C(0x8899aabbccddeeff) ||
        parsed->toString() != canonical)
    {
        throw std::runtime_error(
            "AssetId canonical serialization round trip failed");
    }

    const std::optional<asset::AssetId> uppercase = asset::AssetId::parse(
        "00112233-4455-6677-8899-AABBCCDDEEFF");
    if (!uppercase || *uppercase != *parsed ||
        asset::AssetId::parse("00112233445566778899aabbccddeeff") ||
        asset::AssetId::parse("00112233-4455-6677-8899-aabbccddeefg"))
    {
        throw std::runtime_error("AssetId parsing validation failed");
    }

    const std::optional<asset::AssetId> nil = asset::AssetId::parse(
        "00000000-0000-0000-0000-000000000000");
    if (!nil || nil->valid())
    {
        throw std::runtime_error("AssetId nil representation is invalid");
    }

    std::unordered_set<asset::AssetId> generatedIds;
    for (uint32_t index = 0; index < 64; ++index)
    {
        const asset::AssetId generated = asset::AssetId::generate();
        const std::optional<asset::AssetId> roundTrip =
            asset::AssetId::parse(generated.toString());
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
    constexpr render::MaterialKey olderMaterial =
        render::makeMaterialKey(asset::MaterialAssetHandle{4, 1});
    constexpr render::MaterialKey newerMaterial =
        render::makeMaterialKey(asset::MaterialAssetHandle{4, 2});
    if (!render::MaterialKeyLess{}(olderMaterial, newerMaterial) ||
        render::MaterialKeyLess{}(newerMaterial, olderMaterial))
    {
        throw std::runtime_error(
            "MaterialKey comparison is not deterministic");
    }

    asset::MaterialRenderState opaqueState = asset::makeOpaqueMaterialState();
    const render::PipelineVariantKey opaque = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        opaqueState);

    asset::MaterialRenderState alphaClipState = opaqueState;
    alphaClipState.alphaClipEnabled = true;
    const render::PipelineVariantKey alphaClip = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        alphaClipState);
    alphaClipState.alphaClipThreshold = 0.25f;
    const render::PipelineVariantKey alphaClipWithDifferentThreshold =
        render::makePipelineVariantKey(
            asset::MaterialTemplateAssetHandle{1, 1},
            alphaClipState);
    if (alphaClip != alphaClipWithDifferentThreshold)
    {
        throw std::runtime_error(
            "alpha-clip threshold incorrectly changes PipelineVariantKey");
    }

    asset::MaterialRenderState transparentState =
        asset::makeTransparentMaterialState();
    const render::PipelineVariantKey transparent = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        transparentState);

    if (opaque == alphaClip || opaque == transparent ||
        !render::PipelineVariantKeyLess{}(opaque, alphaClip) ||
        render::PipelineVariantKeyLess{}(alphaClip, opaque) ||
        !render::PipelineVariantKeyLess{}(alphaClip, transparent) ||
        render::PipelineVariantKeyLess{}(transparent, alphaClip))
    {
        throw std::runtime_error(
            "PipelineVariantKey comparison produced an invalid order");
    }

    asset::MaterialRenderState depthDisabledA = opaqueState;
    depthDisabledA.depth.testEnabled = false;
    depthDisabledA.depth.writeEnabled = false;
    depthDisabledA.depth.compare = asset::DepthCompare::Never;
    asset::MaterialRenderState depthDisabledB = depthDisabledA;
    depthDisabledB.depth.compare = asset::DepthCompare::Greater;
    if (render::makePipelineVariantKey(
            asset::MaterialTemplateAssetHandle{1, 1},
            depthDisabledA) !=
        render::makePipelineVariantKey(
            asset::MaterialTemplateAssetHandle{1, 1},
            depthDisabledB))
    {
        throw std::runtime_error(
            "ignored depth compare operation was not canonicalized");
    }
}

void validateRenderItemComparators()
{
    if (render::detail::opaqueDepthSortBucket(0.125f) !=
            render::detail::opaqueDepthSortBucket(0.499f) ||
        render::detail::opaqueDepthSortBucket(0.5f) !=
            render::detail::opaqueDepthSortBucket(1.999f) ||
        render::detail::opaqueDepthSortBucket(2.0f) !=
            render::detail::opaqueDepthSortBucket(7.999f) ||
        render::detail::opaqueDepthSortBucket(8.0f) !=
            render::detail::opaqueDepthSortBucket(31.999f) ||
        render::detail::opaqueDepthSortBucket(0.499f) >=
            render::detail::opaqueDepthSortBucket(0.5f) ||
        render::detail::opaqueDepthSortBucket(1.999f) >=
            render::detail::opaqueDepthSortBucket(2.0f) ||
        render::detail::opaqueDepthSortBucket(7.999f) >=
            render::detail::opaqueDepthSortBucket(8.0f))
    {
        throw std::runtime_error(
            "opaque depth bucket quantization produced invalid boundaries");
    }

    render::RenderItem regularNear{};
    regularNear.material = asset::MaterialAssetHandle{1, 1};
    regularNear.materialKey = render::makeMaterialKey(regularNear.material);
    regularNear.mesh = asset::MeshAssetHandle{1, 1};
    regularNear.pipelineKey = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        asset::makeOpaqueMaterialState());
    regularNear.queue = render::RenderQueue::Opaque;
    regularNear.viewDepth = 2.0f;
    regularNear.candidateIndex = 2;

    render::RenderItem regularFar = regularNear;
    regularFar.viewDepth = 10.0f;
    regularFar.candidateIndex = 3;

    render::RenderItem lowerMaterial = regularNear;
    lowerMaterial.material = asset::MaterialAssetHandle{0, 1};
    lowerMaterial.materialKey = render::makeMaterialKey(
        lowerMaterial.material);
    lowerMaterial.viewDepth = 20.0f;
    lowerMaterial.candidateIndex = 1;

    asset::MaterialRenderState doubleSidedState = asset::makeOpaqueMaterialState();
    doubleSidedState.doubleSided = true;
    render::RenderItem alternatePipeline = regularNear;
    alternatePipeline.pipelineKey = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        doubleSidedState);
    alternatePipeline.material = asset::MaterialAssetHandle{9, 1};
    alternatePipeline.materialKey = render::makeMaterialKey(
        alternatePipeline.material);
    alternatePipeline.viewDepth = 15.0f;
    alternatePipeline.candidateIndex = 5;

    render::RenderItem alphaClip = regularNear;
    alphaClip.queue = render::RenderQueue::AlphaClip;
    asset::MaterialRenderState alphaClipState = asset::makeOpaqueMaterialState();
    alphaClipState.alphaClipEnabled = true;
    alphaClip.pipelineKey = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        alphaClipState);
    alphaClip.viewDepth = 1.0f;
    alphaClip.candidateIndex = 4;

    std::vector<render::RenderItem> opaque = {
        alphaClip,
        regularFar,
        regularNear,
        alternatePipeline,
        lowerMaterial
    };
    std::sort(
        opaque.begin(),
        opaque.end(),
        render::OpaqueRenderItemComparator{});
    if (opaque[0].candidateIndex != 2 ||
        opaque[1].candidateIndex != 5 ||
        opaque[2].candidateIndex != 1 ||
        opaque[3].candidateIndex != 3 ||
        opaque[4].candidateIndex != 4)
    {
        throw std::runtime_error(
            "opaque RenderItem comparator produced an invalid order");
    }

    render::RenderItem transparentNear = regularNear;
    transparentNear.queue = render::RenderQueue::Transparent;
    transparentNear.pipelineKey = render::makePipelineVariantKey(
        asset::MaterialTemplateAssetHandle{1, 1},
        asset::makeTransparentMaterialState());
    transparentNear.candidateIndex = 7;

    render::RenderItem transparentFarHighMaterial = transparentNear;
    transparentFarHighMaterial.viewDepth = 10.0f;
    transparentFarHighMaterial.candidateIndex = 6;

    render::RenderItem transparentFarLowMaterial =
        transparentFarHighMaterial;
    transparentFarLowMaterial.material = asset::MaterialAssetHandle{0, 1};
    transparentFarLowMaterial.materialKey = render::makeMaterialKey(
        transparentFarLowMaterial.material);
    transparentFarLowMaterial.candidateIndex = 5;

    std::vector<render::RenderItem> transparent = {
        transparentNear,
        transparentFarHighMaterial,
        transparentFarLowMaterial
    };
    std::sort(
        transparent.begin(),
        transparent.end(),
        render::TransparentRenderItemComparator{});
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
    render::Camera testCamera;
    render::Camera::Config cameraConfig{};
    cameraConfig.fov = 90.0f;
    cameraConfig.aspectRatio = 1.0f;
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 10.0f;
    testCamera.setConfig(cameraConfig);

    render::RenderView view = testCamera.makeRenderView();
    view.cullingMask = render::RenderLayer::World;
    view.cullingFlags = render::CullingFlags::All;

    std::vector<render::RenderCandidate> candidates(4);
    candidates[0].worldBounds = {
        {-0.1f, -0.1f, -1.1f},
        { 0.1f,  0.1f, -0.9f}
    };
    candidates[1].worldBounds = {
        {-0.1f, -0.1f, 0.9f},
        { 0.1f,  0.1f, 1.1f}
    };
    candidates[2].worldBounds = candidates[1].worldBounds;
    candidates[2].boundsCullingMode = render::BoundsCullingMode::Disabled;
    candidates[3].worldBounds = candidates[0].worldBounds;
    candidates[3].layerMask = render::RenderLayer::Editor;

    const render::CullingResults culled =
        render::CullingSystem{}.cull(candidates, view);
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

    view.cullingFlags = render::CullingFlags::None;
    const render::CullingResults unculled =
        render::CullingSystem{}.cull(candidates, view);
    if (unculled.visibleCount() != candidates.size() ||
        unculled.layerCulledCount != 0 ||
        unculled.frustumCulledCount != 0)
    {
        throw std::runtime_error(
            "disabled culling did not preserve every candidate");
    }

    render::Camera otherCamera;
    if (otherCamera.viewId() == testCamera.viewId())
    {
        throw std::runtime_error("different cameras reused a RenderViewId");
    }
    const uint64_t previousRevision = testCamera.gpuDataRevision();
    testCamera.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    const render::RenderView changedView = testCamera.makeRenderView();
    if (changedView.id != view.id ||
        changedView.gpuDataRevision == previousRevision)
    {
        throw std::runtime_error(
            "camera RenderView identity or GPU revision is invalid");
    }
}

void validateOfflineMaterialAssets(
    asset::AssetManager& assets,
    const editor::DemoContent& content,
    const importer::texture::TextureImportRegistry& textureImports)
{
    const asset::TextureAsset& flatNormal = assets.texture(
        content.defaultNormalTexture);
    if (flatNormal.colorSpace() != asset::TextureColorSpace::Linear ||
        flatNormal.payload().size() != 4 ||
        flatNormal.payload()[0] != std::byte{0x80} ||
        flatNormal.payload()[1] != std::byte{0x80} ||
        flatNormal.payload()[2] != std::byte{0xff})
    {
        throw std::runtime_error(
            "default normal texture is not a linear flat tangent-space normal");
    }

    bool foundImportedPbrMaterial = false;
    const asset::ModelAsset& model = assets.model(content.model);
    for (const asset::ModelNode& node : model.nodes())
    {
        for (asset::MeshAssetHandle meshHandle : node.meshes)
        {
            for (const asset::SubmeshData& submesh :
                 assets.mesh(meshHandle).submeshes())
            {
                const asset::MaterialAsset& material =
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
                if (colorSpace(0) != asset::TextureColorSpace::Srgb ||
                    colorSpace(1) != asset::TextureColorSpace::Linear ||
                    colorSpace(2) != asset::TextureColorSpace::Linear ||
                    colorSpace(3) != asset::TextureColorSpace::Linear ||
                    colorSpace(4) != asset::TextureColorSpace::Srgb)
                {
                    throw std::runtime_error(
                        "glTF material textures use incorrect semantic color spaces");
                }

                const auto validateImportSettings =
                    [&](uint32_t slot,
                        asset::TextureColorSpace expectedColorSpace,
                        asset::TextureFormat expectedFormat,
                        bool expectedNormalMap)
                    {
                        const asset::TextureAssetHandle texture =
                            material.textures()[slot];
                        const importer::texture::TextureImportRecord* record =
                            textureImports.find(texture);
                        if (record == nullptr)
                        {
                            if (texture != content.defaultTexture &&
                                texture != content.defaultDataTexture &&
                                texture != content.defaultNormalTexture)
                            {
                                throw std::runtime_error(
                                    "imported material texture has no reimport provenance");
                            }
                            return;
                        }
                        if (
                            record->settings.colorSpace != expectedColorSpace ||
                            !record->settings.generateMipmaps ||
                            record->settings.basis.encoding !=
                                importer::texture::KtxPayloadEncoding::Uastc ||
                            record->settings.basis.normalMap !=
                                expectedNormalMap ||
                            record->settings.transcodeFormat != expectedFormat ||
                            !record->settings.highQualityTranscode)
                        {
                            throw std::runtime_error(
                                "DemoContent texture import policy produced incorrect settings");
                        }
                    };
                validateImportSettings(
                    0,
                    asset::TextureColorSpace::Srgb,
                    asset::TextureFormat::BC7UNorm,
                    false);
                validateImportSettings(
                    1,
                    asset::TextureColorSpace::Linear,
                    asset::TextureFormat::BC7UNorm,
                    false);
                validateImportSettings(
                    2,
                    asset::TextureColorSpace::Linear,
                    asset::TextureFormat::BC5UNorm,
                    true);
                validateImportSettings(
                    3,
                    asset::TextureColorSpace::Linear,
                    asset::TextureFormat::BC7UNorm,
                    false);
                validateImportSettings(
                    4,
                    asset::TextureColorSpace::Srgb,
                    asset::TextureFormat::BC7UNorm,
                    false);
            }
        }
    }
    if (!foundImportedPbrMaterial)
    {
        throw std::runtime_error(
            "demo model contains no imported PBR material to validate");
    }

    const asset::ShaderAsset& fragmentShader = assets.shader(
        content.pbrFragmentShader);
    const auto materialDescriptor = std::find_if(
        fragmentShader.interface().descriptorBindings.begin(),
        fragmentShader.interface().descriptorBindings.end(),
        [](const asset::ShaderDescriptorBindingDesc& descriptor)
        {
            return descriptor.set == 1 && descriptor.binding == 0 &&
                descriptor.type == asset::ShaderDescriptorType::UniformBuffer;
        });
    if (fragmentShader.interface().interfaceHash == 0 ||
        materialDescriptor ==
            fragmentShader.interface().descriptorBindings.end() ||
        !materialDescriptor->bufferLayout ||
        materialDescriptor->bufferLayout->minimumByteSize != 36 ||
        materialDescriptor->bufferLayout->members.size() != 4)
    {
        throw std::runtime_error(
            "CPU-only SPIR-V reflection produced an invalid material interface");
    }

    const asset::MaterialTemplateAsset& materialTemplate =
        assets.materialTemplate(content.materialTemplate);
    asset::MaterialTemplateAsset::CreateInfo templateInfo{};
    templateInfo.name = materialTemplate.name();
    templateInfo.shaders = materialTemplate.shaders();
    templateInfo.parameterBlock = materialTemplate.parameterBlock();
    templateInfo.parameterDataSize = materialTemplate.parameterDataSize();
    templateInfo.parameters = materialTemplate.parameters();
    templateInfo.textureSlots = materialTemplate.textureSlots();

    const asset::ValidationReport currentTemplateReport =
        assets.validateMaterialTemplate(templateInfo);
    if (!currentTemplateReport.valid() ||
        hasValidationIssue(
            currentTemplateReport,
            "Template.InactiveImageBinding",
            asset::ValidationSeverity::Warning) ||
        !assets.isMaterialTemplateCurrent(content.materialTemplate))
    {
        throw std::runtime_error(
            "valid material template did not pass offline validation");
    }

    asset::MaterialTemplateAsset::CreateInfo invalidTemplate = templateInfo;
    invalidTemplate.parameters.back().byteOffset = 36;
    const asset::ValidationReport invalidTemplateReport =
        assets.validateMaterialTemplate(invalidTemplate);
    if (invalidTemplateReport.valid() ||
        !hasValidationIssue(
            invalidTemplateReport,
            "Template.ParameterOffsetMismatch",
            asset::ValidationSeverity::Error))
    {
        throw std::runtime_error(
            "invalid material template passed shader-interface validation");
    }

    asset::MaterialAsset::CreateInfo invalidMaterial{};
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
    const asset::ValidationReport invalidMaterialReport =
        assets.validateMaterial(invalidMaterial);
    if (invalidMaterialReport.valid() ||
        !hasValidationIssue(
            invalidMaterialReport,
            "Material.ParameterTypeMismatch",
            asset::ValidationSeverity::Error))
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
    catch (const asset::AssetValidationError& error)
    {
        creationRejected = hasValidationIssue(
            error.report(),
            "Material.ParameterTypeMismatch",
            asset::ValidationSeverity::Error);
    }
    if (!creationRejected)
    {
        throw std::runtime_error(
            "AssetManager accepted an invalid material instance");
    }
}


void validateRenderFrame(const render::RenderFrame& renderFrame)
{
    if (renderFrame.renderList.empty())
    {
        throw std::runtime_error(
            "scene extraction produced no render objects");
    }

    const auto validateItems = [&](const auto& items)
    {
        for (const render::RenderItem& item : items)
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
    const render::RenderFrame& renderFrame,
    const rhi::vulkan::RenderAssetCache& renderAssets)
{
    const rhi::vulkan::VulkanDrawList resolved = rhi::vulkan::VulkanDrawListCompiler{}.compile(
        renderFrame.renderList,
        renderAssets);
    if (resolved.size() != renderFrame.renderList.size())
    {
        throw std::runtime_error(
            "Vulkan draw-list compilation changed the draw count");
    }

    render::RenderList staleList = renderFrame.renderList;
    render::RenderItem* staleItem = !staleList.opaque.empty()
        ? &staleList.opaque.front()
        : &staleList.transparent.front();
    ++staleItem->mesh.generation;

    bool staleHandleRejected = false;
    try
    {
        static_cast<void>(rhi::vulkan::VulkanDrawListCompiler{}.compile(
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
    const asset::TextureFormatInfo rgba8 =
        asset::textureFormatInfo(asset::TextureFormat::RGBA8UNorm);
    const asset::TextureFormatInfo bc1 =
        asset::textureFormatInfo(asset::TextureFormat::BC1RGBAUNorm);
    const asset::TextureFormatInfo bc7 =
        asset::textureFormatInfo(asset::TextureFormat::BC7UNorm);
    if (rgba8.compressed || rgba8.bytesPerBlock != 4 ||
        !rgba8.supportsSrgb ||
        !bc1.compressed || bc1.blockWidth != 4 ||
        bc1.blockHeight != 4 || bc1.bytesPerBlock != 8 ||
        !bc1.supportsSrgb ||
        !bc7.compressed || bc7.bytesPerBlock != 16 ||
        !bc7.supportsSrgb ||
        textureMipByteSize(asset::TextureFormat::RGBA8UNorm, 4, 4) != 64 ||
        textureMipByteSize(asset::TextureFormat::BC1RGBAUNorm, 7, 5) != 32 ||
        textureMipByteSize(asset::TextureFormat::BC7UNorm, 2, 2) != 16)
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

    importer::texture::KtxTextureCooker::Request request{};
    request.inputPath = inputPath;
    request.outputPath = outputPath;
    request.colorSpace = asset::TextureColorSpace::Srgb;
    request.generateMipmaps = true;
    request.mipFilter = importer::texture::TextureMipFilter::Mitchell;
    request.mipEdgeMode = importer::texture::TextureMipEdgeMode::Clamp;
    request.basis.encoding = importer::texture::KtxPayloadEncoding::Uastc;
    request.basis.uastcQualityLevel = 0;
    request.basis.threadCount = 1;
    request.zstdLevel = 1;

    importer::texture::KtxTextureImporter::CreateInfo importInfo{};
    importInfo.name = "toktx file pipeline smoke texture";
    importInfo.transcodeFormat = asset::TextureFormat::BC7UNorm;
    const asset::TextureAsset::CreateInfo textureInfo =
        importer::texture::KtxTextureCooker{}.cookAndImport(request, importInfo);
    if (!std::filesystem::is_regular_file(outputPath) ||
        textureInfo.format != asset::TextureFormat::BC7UNorm ||
        textureInfo.colorSpace != asset::TextureColorSpace::Srgb ||
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
        const asset::TextureFormat format = basisEncoded
            ? asset::TextureFormat::RGBA8UNorm
            : asset::TextureFormat::BC7UNorm;
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

void validateKtxTextureImportAndUpload(const rhi::vulkan::Device& device)
{
    const importer::texture::KtxTextureImporter importer;
    importer::texture::KtxTextureImporter::CreateInfo importInfo{};
    importInfo.name = "Direct BC7 KTX2 smoke texture";

    const std::vector<uint8_t> directBytes = makeKtx2Fixture(false);
    asset::TextureAsset::CreateInfo directInfo = importer.importMemory(
        directBytes.data(),
        directBytes.size(),
        importInfo);
    if (directInfo.format != asset::TextureFormat::BC7UNorm ||
        directInfo.colorSpace != asset::TextureColorSpace::Linear ||
        directInfo.payload.size() != 48 ||
        directInfo.mipLevels.size() != 3)
    {
        throw std::runtime_error(
            "direct BC7 KTX2 import produced invalid asset metadata");
    }

    importInfo.name = "Basis-to-BC7 KTX2 smoke texture";
    importInfo.transcodeFormat = asset::TextureFormat::BC7UNorm;
    const std::vector<uint8_t> basisBytes = makeKtx2Fixture(true);
    asset::TextureAsset::CreateInfo textureInfo = importer.importMemory(
        basisBytes.data(),
        basisBytes.size(),
        importInfo);
    if (textureInfo.format != asset::TextureFormat::BC7UNorm ||
        textureInfo.colorSpace != asset::TextureColorSpace::Linear ||
        textureInfo.payload.size() != 48 ||
        textureInfo.mipLevels.size() != 3)
    {
        throw std::runtime_error(
            "Basis-to-BC7 KTX2 transcode produced invalid asset metadata");
    }

    rhi::vulkan::CommandPool commandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    rhi::vulkan::UploadContext uploadContext(device, commandPool);

    const asset::TextureAsset texture(std::move(textureInfo));
    rhi::vulkan::GpuTexture::CreateInfo gpuTextureInfo{};
    gpuTextureInfo.asset = &texture;
    gpuTextureInfo.viewRange.baseMipLevel = 1;
    gpuTextureInfo.viewRange.levelCount = 2;
    const rhi::vulkan::GpuTexture gpuTexture(device, uploadContext, gpuTextureInfo);
    if (!gpuTexture || gpuTexture.format() != VK_FORMAT_BC7_UNORM_BLOCK)
    {
        throw std::runtime_error(
            "transcoded BC7 KTX2 upload produced an invalid GPU texture");
    }
}

asset::TextureAsset::CreateInfo cloneTextureCreateInfo(
    const asset::TextureAsset& source,
    std::string name)
{
    asset::TextureAsset::CreateInfo result{};
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
    asset::AssetManager assets;
    asset::TextureAsset::CreateInfo original{};
    original.name = "Original";
    original.width = 1;
    original.height = 1;
    original.format = asset::TextureFormat::RGBA8UNorm;
    original.payload = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0xff}};
    const asset::TextureAssetHandle handle =
        assets.createTexture(std::move(original));

    asset::TextureAsset::CreateInfo replacement = cloneTextureCreateInfo(
        assets.texture(handle),
        "Replacement");
    replacement.payload[0] = std::byte{0x80};
    asset::TextureAsset previous = assets.replaceTexture(
        handle,
        asset::TextureAsset(std::move(replacement)));
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
    const rhi::vulkan::Device& device,
    asset::AssetManager& assets,
    rhi::vulkan::RenderAssetCache& renderAssets,
    asset::TextureAssetHandle handle)
{
    const rhi::vulkan::GpuTexture* originalGpu = renderAssets.tryTexture(handle);
    if (originalGpu == nullptr)
    {
        throw std::runtime_error(
            "GPU texture replacement test requires a cached texture");
    }
    const VkImageView originalView = originalGpu->view();

    asset::TextureAsset replacementAsset(cloneTextureCreateInfo(
        assets.texture(handle),
        "GPU replacement texture"));
    rhi::vulkan::CommandPool commandPool(
        device,
        device.graphicsQueueFamily(),
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    rhi::vulkan::UploadContext uploadContext(device, commandPool);
    rhi::vulkan::GpuTexture staged = renderAssets.stageTextureReplacement(
        device,
        uploadContext,
        replacementAsset);
    rhi::vulkan::GpuTexture previousGpu = renderAssets.commitTextureReplacement(
        device,
        assets,
        handle,
        std::move(staged));
    asset::TextureAsset previousAsset = assets.replaceTexture(
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

void AppSmokeTests::runShaderAssetTest(
    const std::filesystem::path& shaderPath)
{
    validateShaderAssetPipeline(shaderPath);
}

void AppSmokeTests::runRenderTest()
{
    editor::App app;
    editor::RuntimeGui gui;
    runRenderTest(app, editor::App::RunConfig{}, gui);
}

void AppSmokeTests::runRenderTest(
    editor::App& app,
    const editor::App::RunConfig& config,
    editor::ApplicationGui& gui)
{
    app.initWindow(config, false);
    app.initVulkan(config);
    validateKtxTextureImportAndUpload(app.vulkanContext.device());
    app.initImGui(config);
    app.guiRenderBridge.attach(app.renderer, app.renderAssets);
    editor::ApplicationGuiContext guiContext{
        app.assetManager,
        app.scene,
        app.guiRenderBridge
    };
    gui.attach(guiContext);
    asset::TextureAssetHandle editorPreviewTexture{};
    const std::vector<render::RenderCandidate> previewCandidates =
        render::SceneRenderExtractor{}.extract(app.scene, app.assetManager);
    for (const render::RenderCandidate& candidate : previewCandidates)
    {
        const asset::MaterialAsset& material =
            app.assetManager.material(candidate.material);
        const auto texture = std::find_if(
            material.textures().begin(),
            material.textures().end(),
            [&](asset::TextureAssetHandle handle)
            {
                return app.renderAssets.tryTexture(handle) != nullptr &&
                    app.textureImports.find(handle) != nullptr;
            });
        if (texture != material.textures().end())
        {
            editorPreviewTexture = *texture;
            break;
        }
    }
    if (!editorPreviewTexture)
    {
        throw std::runtime_error(
            "render test found no uploaded source-backed texture");
    }
    validateGpuTextureReplacement(
        app.vulkanContext.device(),
        app.assetManager,
        app.renderAssets,
        editorPreviewTexture);
    if (config.outputMode == rhi::vulkan::VulkanRenderer::OutputMode::Editor)
    {
        const importer::texture::TextureImportRecord* before =
            app.textureImports.find(editorPreviewTexture);
        if (before == nullptr)
        {
            throw std::runtime_error(
                "Editor texture reimport test requires source provenance");
        }
        const uint64_t previousRevision = before->revision;
        importer::texture::TextureImportSettings asynchronousSettings = before->settings;
        asynchronousSettings.basis.encoding = importer::texture::KtxPayloadEncoding::Uastc;
        asynchronousSettings.basis.uastcQualityLevel = 0;
        asynchronousSettings.zstdLevel = 0;
        asynchronousSettings.transcodeFormat = asset::TextureFormat::BC7UNorm;
        app.pendingTextureReimports_.push_back(importer::texture::TextureReimportRequest{
            editorPreviewTexture,
            std::move(asynchronousSettings)});
        app.processPendingTextureReimport();

        const importer::texture::TextureImportRecord* started =
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
            const importer::texture::TextureImportRecord* current =
                app.textureImports.find(editorPreviewTexture);
            if (current == nullptr || !current->reimporting)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const importer::texture::TextureImportRecord* after =
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
        if (config.outputMode == rhi::vulkan::VulkanRenderer::OutputMode::Editor)
        {
            const render::ApplicationGuiTexture smokeTexturePreview =
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
            if (config.outputMode == rhi::vulkan::VulkanRenderer::OutputMode::Editor)
            {
                const render::ApplicationGuiTexture smokeTexturePreview =
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

            const render::RenderFrame renderFrame = app.makeRenderFrame();
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
                rhi::vulkan::VulkanRenderer::RenderResult::NeedsResize)
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

} // namespace test
} // namespace rubia
