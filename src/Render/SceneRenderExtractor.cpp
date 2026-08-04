#include "Render/SceneRenderExtractor.h"

#include "GpuMaterial.h"
#include "Mesh.h"

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

RenderFrame SceneRenderExtractor::extract(
    const Scene& scene,
    const AssetManager& assets,
    const RenderAssetCache& renderAssets,
    RenderView view) const
{
    RenderFrame frame{};
    frame.view = std::move(view);

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
                const Mesh& mesh = renderAssets.mesh(meshHandle);
                for (uint32_t submeshIndex = 0;
                     submeshIndex < mesh.submeshes().size();
                     ++submeshIndex)
                {
                    const SubmeshData& submesh =
                        mesh.submeshes()[submeshIndex];
                    RenderObject object{};
                    object.mesh = &mesh;
                    object.submeshIndex = submeshIndex;
                    object.material =
                        &renderAssets.material(submesh.material);
                    object.objectData.world = modelWorld[modelIndex];
                    object.objectData.normalMatrix = glm::transpose(
                        glm::inverse(object.objectData.world));
                    frame.objects.push_back(object);
                }
            }
        }
    }
    return frame;
}

} // namespace VkRenderer
