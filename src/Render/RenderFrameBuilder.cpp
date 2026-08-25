#include "Render/RenderFrameBuilder.h"

#include "Render/CullingSystem.h"
#include "Render/RenderListBuilder.h"
#include "Render/SceneRenderExtractor.h"

#include <utility>
#include <vector>

namespace VkRenderer
{

RenderFrame buildRenderFrame(
    const Scene& scene,
    const AssetManager& assets,
    RenderView view)
{
    std::vector<RenderCandidate> candidates =
        SceneRenderExtractor{}.extract(scene, assets);
    const CullingResults cullingResults =
        CullingSystem{}.cull(candidates, view);

    RenderFrame frame{};
    frame.renderList = RenderListBuilder{}.build(
        candidates,
        cullingResults,
        view,
        assets);
    frame.view = std::move(view);
    return frame;
}

} // namespace VkRenderer
