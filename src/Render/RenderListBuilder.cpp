#include "Render/RenderListBuilder.h"

#include "Asset/AssetManager.h"
#include "GpuMaterial.h"
#include "Mesh.h"
#include "RenderAssetCache.h"
#include "Render/RenderItemComparator.h"
#include "Render/RenderView.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace VkRenderer
{

RenderList RenderListBuilder::build(
    const std::vector<RenderCandidate>& candidates,
    const CullingResults& cullingResults,
    const RenderView& view,
    const AssetManager& assets,
    const RenderAssetCache& renderAssets) const
{
    RenderList result{};
    result.objectData.reserve(cullingResults.visibleCandidateIndices.size());
    result.opaque.reserve(cullingResults.visibleCandidateIndices.size());
    result.transparent.reserve(
        cullingResults.visibleCandidateIndices.size());

    for (uint32_t index : cullingResults.visibleCandidateIndices)
    {
        if (index >= candidates.size())
        {
            throw std::out_of_range(
                "culling result references an invalid render candidate");
        }

        const RenderCandidate& candidate = candidates[index];
        if (!candidate.mesh || !candidate.material)
        {
            throw std::invalid_argument(
                "render candidate has an invalid mesh or material handle");
        }

        const MeshAsset& meshAsset = assets.mesh(candidate.mesh);
        if (candidate.submeshIndex >= meshAsset.submeshes().size())
        {
            throw std::out_of_range(
                "render candidate references an invalid submesh");
        }
        if (!candidate.worldBounds.valid())
        {
            throw std::invalid_argument(
                "render candidate has invalid world bounds");
        }

        const MaterialAsset& materialAsset =
            assets.material(candidate.material);
        const MaterialRenderState& renderState =
            materialAsset.renderState();
        const RenderQueue queue = renderQueueFor(renderState);
        const glm::vec4 viewCenter = view.viewMatrix * glm::vec4(
            candidate.worldBounds.center(),
            1.0f);
        const float viewDepth = -viewCenter.z;
        if (!std::isfinite(viewDepth))
        {
            throw std::invalid_argument(
                "render candidate produced a non-finite view depth");
        }

        RenderItem item{};
        item.mesh = &renderAssets.mesh(candidate.mesh);
        item.material = &renderAssets.material(candidate.material);
        item.meshHandle = candidate.mesh;
        item.materialHandle = candidate.material;
        item.materialKey = makeMaterialKey(candidate.material);
        item.pipelineKey = makePipelineVariantKey(
            materialAsset.materialTemplate(),
            renderState);
        item.candidateIndex = index;
        item.submeshIndex = candidate.submeshIndex;
        item.objectIndex = static_cast<uint32_t>(result.objectData.size());
        item.queue = queue;
        item.viewDepth = viewDepth;
        result.objectData.push_back(candidate.objectData);

        if (isTransparentQueue(queue))
        {
            result.transparent.push_back(item);
        }
        else
        {
            result.opaque.push_back(item);
        }
    }

    std::sort(
        result.opaque.begin(),
        result.opaque.end(),
        OpaqueRenderItemComparator{});
    std::sort(
        result.transparent.begin(),
        result.transparent.end(),
        TransparentRenderItemComparator{});
    return result;
}

} // namespace VkRenderer
