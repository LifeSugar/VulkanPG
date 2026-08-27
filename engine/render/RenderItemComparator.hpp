#pragma once

#include "render/RenderItem.hpp"

#include <cstdint>
#include <cstring>
#include <limits>

namespace rubia::render
{
namespace detail
{

template <typename Asset>
[[nodiscard]] constexpr bool assetHandleLess(
    asset::AssetHandle<Asset> left,
    asset::AssetHandle<Asset> right) noexcept
{
    if (left.index != right.index)
    {
        return left.index < right.index;
    }
    return left.generation < right.generation;
}

/// Roughly orders positive view depths front-to-back using the same
/// high-byte float quantization as Unity 2019.4. Each bucket spans two
/// consecutive IEEE-754 exponents, giving boundaries near
/// 0.125, 0.5, 2, 8, 32, 128, ... world units.
[[nodiscard]] inline uint32_t opaqueDepthSortBucket(
    float viewDepth) noexcept
{
    const float nonNegativeDepth = viewDepth > 0.0f ? viewDepth : 0.0f;
    uint32_t depthBits = 0;
    static_assert(
        sizeof(depthBits) == sizeof(nonNegativeDepth) &&
            std::numeric_limits<float>::is_iec559,
        "opaque depth quantization requires IEEE-754 32-bit floats");
    std::memcpy(&depthBits, &nonNegativeDepth, sizeof(depthBits));
    return depthBits >> 24;
}

} // namespace detail

/// Orders opaque geometry by coarse front-to-back buckets, then groups state.
struct OpaqueRenderItemComparator
{
    [[nodiscard]] bool operator()(
        const RenderItem& left,
        const RenderItem& right) const noexcept
    {
        if (left.queue != right.queue)
        {
            return renderQueueValue(left.queue) <
                renderQueueValue(right.queue);
        }
        const uint32_t leftDepthBucket =
            detail::opaqueDepthSortBucket(left.viewDepth);
        const uint32_t rightDepthBucket =
            detail::opaqueDepthSortBucket(right.viewDepth);
        if (leftDepthBucket != rightDepthBucket)
        {
            return leftDepthBucket < rightDepthBucket;
        }
        if (left.pipelineKey != right.pipelineKey)
        {
            return PipelineVariantKeyLess{}(
                left.pipelineKey,
                right.pipelineKey);
        }
        if (left.materialKey != right.materialKey)
        {
            return MaterialKeyLess{}(
                left.materialKey,
                right.materialKey);
        }
        if (left.mesh != right.mesh)
        {
            return detail::assetHandleLess(
                left.mesh,
                right.mesh);
        }
        return left.candidateIndex < right.candidateIndex;
    }
};

/// Preserves transparent queue order and blends geometry back-to-front.
struct TransparentRenderItemComparator
{
    [[nodiscard]] constexpr bool operator()(
        const RenderItem& left,
        const RenderItem& right) const noexcept
    {
        if (left.queue != right.queue)
        {
            return renderQueueValue(left.queue) <
                renderQueueValue(right.queue);
        }
        if (left.viewDepth != right.viewDepth)
        {
            return left.viewDepth > right.viewDepth;
        }
        if (left.pipelineKey != right.pipelineKey)
        {
            return PipelineVariantKeyLess{}(
                left.pipelineKey,
                right.pipelineKey);
        }
        if (left.materialKey != right.materialKey)
        {
            return MaterialKeyLess{}(
                left.materialKey,
                right.materialKey);
        }
        if (left.mesh != right.mesh)
        {
            return detail::assetHandleLess(
                left.mesh,
                right.mesh);
        }
        return left.candidateIndex < right.candidateIndex;
    }
};

} // namespace rubia::render
