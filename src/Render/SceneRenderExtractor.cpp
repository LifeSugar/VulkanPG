#include "Render/SceneRenderExtractor.h"

#include "Asset/AssetManager.h"
#include "Scene/Scene.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

template <typename Node, uint32_t InvalidParent>
std::vector<glm::mat4> hierarchyWorldTransforms(
    const std::vector<Node>& nodes,
    const glm::mat4& rootTransform)
{
    std::vector<glm::mat4> world(nodes.size(), glm::mat4(1.0f));
    std::vector<uint8_t> states(nodes.size(), 0);

    std::function<const glm::mat4&(uint32_t)> resolve =
        [&](uint32_t index) -> const glm::mat4&
        {
            if (states[index] == 2)
            {
                return world[index];
            }
            if (states[index] == 1)
            {
                throw std::logic_error(
                    "validated scene/model hierarchy contains a cycle");
            }

            states[index] = 1;
            const uint32_t parent = nodes[index].parent;
            world[index] = parent == InvalidParent
                ? rootTransform * nodes[index].localTransform
                : resolve(parent) * nodes[index].localTransform;
            states[index] = 2;
            return world[index];
        };

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        resolve(index);
    }
    return world;
}

} // namespace

std::vector<RenderCandidate> SceneRenderExtractor::extract(
    const Scene& scene,
    const AssetManager& assets) const
{
    std::vector<RenderCandidate> candidates;

    const std::vector<SceneNode>& sceneNodes = scene.nodes();
    const std::vector<glm::mat4> sceneWorld =
        hierarchyWorldTransforms<SceneNode, kInvalidSceneNodeIndex>(
            sceneNodes,
            glm::mat4(1.0f));

    for (std::size_t sceneIndex = 0;
         sceneIndex < sceneNodes.size();
         ++sceneIndex)
    {
        const SceneNode& sceneNode = sceneNodes[sceneIndex];
        if (!sceneNode.model)
        {
            continue;
        }

        const ModelAsset& model = assets.model(sceneNode.model);
        const std::vector<glm::mat4> modelWorld =
            hierarchyWorldTransforms<ModelNode, kInvalidModelNodeIndex>(
                model.nodes(),
                sceneWorld[sceneIndex]);

        for (std::size_t modelIndex = 0;
             modelIndex < model.nodes().size();
             ++modelIndex)
        {
            const ModelNode& modelNode = model.nodes()[modelIndex];
            for (MeshAssetHandle meshHandle : modelNode.meshes)
            {
                const MeshAsset& mesh = assets.mesh(meshHandle);
                for (uint32_t submeshIndex = 0;
                     submeshIndex < mesh.submeshes().size();
                     ++submeshIndex)
                {
                    const SubmeshData& submesh =
                        mesh.submeshes()[submeshIndex];
                    RenderCandidate candidate{};
                    candidate.mesh = meshHandle;
                    candidate.submeshIndex = submeshIndex;
                    candidate.material = submesh.material;
                    candidate.layerMask = sceneNode.layerMask;
                    candidate.boundsCullingMode =
                        sceneNode.boundsCullingMode;
                    candidate.objectData.world = modelWorld[modelIndex];
                    candidate.objectData.normalMatrix = glm::transpose(
                        glm::inverse(candidate.objectData.world));
                    candidate.worldBounds = transformAabb(
                        mesh.localBounds(),
                        candidate.objectData.world);

                    candidates.push_back(candidate);
                }
            }
        }
    }
    return candidates;
}

} // namespace VkRenderer
