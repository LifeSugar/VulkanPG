#include "render/RenderFrameBuilder.hpp"

#include "render/CullingSystem.hpp"
#include "render/RenderListBuilder.hpp"
#include "render/SceneRenderExtractor.hpp"

#include <utility>
#include <vector>

namespace rubia::render
{

RenderFrame buildRenderFrame(
    const scene::Scene& scene,
    const asset::AssetManager& assets,
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

} // namespace rubia::render
