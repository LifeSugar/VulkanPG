#pragma once

#include "Asset/AssetFwd.h"
#include "Asset/MaterialState.h"

#include <cstdint>
#include <type_traits>

namespace VkRenderer
{

/// Shader variants selected while compiling a material pipeline.
enum class ShaderFeatureFlags : uint32_t
{
    None = 0,
    AlphaClip = 1u << 0
};

[[nodiscard]] constexpr ShaderFeatureFlags operator|(
    ShaderFeatureFlags left,
    ShaderFeatureFlags right) noexcept
{
    using Storage = std::underlying_type_t<ShaderFeatureFlags>;
    return static_cast<ShaderFeatureFlags>(
        static_cast<Storage>(left) | static_cast<Storage>(right));
}

[[nodiscard]] constexpr uint32_t shaderFeatureBits(
    ShaderFeatureFlags features) noexcept
{
    return static_cast<uint32_t>(features);
}

/// Backend-independent blend families currently supported by scene materials.
enum class PipelineBlendMode : uint8_t
{
    Disabled,
    Alpha
};

/// Backend-independent face-culling mode used by a pipeline variant.
enum class PipelineCullMode : uint8_t
{
    None,
    Front,
    Back
};

/// Material-controlled subset of a complete graphics-pipeline key.
///
/// Render-pass compatibility, vertex layout, and sample count remain
/// renderer-owned inputs and are combined with this key by the pipeline cache.
struct PipelineVariantKey
{
    MaterialTemplateAssetHandle materialTemplate;
    ShaderFeatureFlags shaderFeatures = ShaderFeatureFlags::None;
    PipelineBlendMode blendMode = PipelineBlendMode::Disabled;
    PipelineCullMode cullMode = PipelineCullMode::Back;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    DepthCompare depthCompare = DepthCompare::LessEqual;

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return materialTemplate.index != kInvalidAssetIndex;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return valid();
    }
};

/// Canonicalizes high-level material state into pipeline-affecting fields.
/// Alpha-clip threshold is deliberately excluded: it is material data, not PSO
/// state. A disabled depth test also canonicalizes its ignored compare op.
[[nodiscard]] constexpr PipelineVariantKey makePipelineVariantKey(
    MaterialTemplateAssetHandle materialTemplate,
    const MaterialRenderState& state) noexcept
{
    PipelineVariantKey key{};
    key.materialTemplate = materialTemplate;
    key.shaderFeatures = state.alphaClipEnabled
        ? ShaderFeatureFlags::AlphaClip
        : ShaderFeatureFlags::None;
    key.blendMode = state.transparent()
        ? PipelineBlendMode::Alpha
        : PipelineBlendMode::Disabled;
    key.cullMode = state.doubleSided
        ? PipelineCullMode::None
        : PipelineCullMode::Back;
    key.depthTestEnabled = state.depth.testEnabled;
    key.depthWriteEnabled = state.depth.writeEnabled;
    key.depthCompare = state.depth.testEnabled
        ? state.depth.compare
        : DepthCompare::Always;
    return key;
}

[[nodiscard]] constexpr bool operator==(
    const PipelineVariantKey& left,
    const PipelineVariantKey& right) noexcept
{
    return left.materialTemplate.index == right.materialTemplate.index &&
        left.materialTemplate.generation ==
            right.materialTemplate.generation &&
        left.shaderFeatures == right.shaderFeatures &&
        left.blendMode == right.blendMode &&
        left.cullMode == right.cullMode &&
        left.depthTestEnabled == right.depthTestEnabled &&
        left.depthWriteEnabled == right.depthWriteEnabled &&
        left.depthCompare == right.depthCompare;
}

[[nodiscard]] constexpr bool operator!=(
    const PipelineVariantKey& left,
    const PipelineVariantKey& right) noexcept
{
    return !(left == right);
}

/// Lexicographically orders every semantic field; no padding bytes or hashes
/// participate, so the result is deterministic across processes and GPUs.
struct PipelineVariantKeyLess
{
    [[nodiscard]] constexpr bool operator()(
        const PipelineVariantKey& left,
        const PipelineVariantKey& right) const noexcept
    {
        if (left.materialTemplate.index != right.materialTemplate.index)
        {
            return left.materialTemplate.index < right.materialTemplate.index;
        }
        if (left.materialTemplate.generation !=
            right.materialTemplate.generation)
        {
            return left.materialTemplate.generation <
                right.materialTemplate.generation;
        }
        if (left.blendMode != right.blendMode)
        {
            return static_cast<uint8_t>(left.blendMode) <
                static_cast<uint8_t>(right.blendMode);
        }
        if (left.shaderFeatures != right.shaderFeatures)
        {
            return shaderFeatureBits(left.shaderFeatures) <
                shaderFeatureBits(right.shaderFeatures);
        }
        if (left.cullMode != right.cullMode)
        {
            return static_cast<uint8_t>(left.cullMode) <
                static_cast<uint8_t>(right.cullMode);
        }
        if (left.depthTestEnabled != right.depthTestEnabled)
        {
            return left.depthTestEnabled < right.depthTestEnabled;
        }
        if (left.depthWriteEnabled != right.depthWriteEnabled)
        {
            return left.depthWriteEnabled < right.depthWriteEnabled;
        }
        return static_cast<uint8_t>(left.depthCompare) <
            static_cast<uint8_t>(right.depthCompare);
    }
};

constexpr MaterialRenderState kPipelineKeyOpaqueState =
    makeOpaqueMaterialState();
constexpr PipelineVariantKey kPipelineKeyOpaque = makePipelineVariantKey(
    MaterialTemplateAssetHandle{1, 1},
    kPipelineKeyOpaqueState);
static_assert(kPipelineKeyOpaque.blendMode == PipelineBlendMode::Disabled);
static_assert(kPipelineKeyOpaque.cullMode == PipelineCullMode::Back);
static_assert(kPipelineKeyOpaque.depthWriteEnabled);

} // namespace VkRenderer
