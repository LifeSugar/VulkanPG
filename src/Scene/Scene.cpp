#include "Scene/Scene.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

void validateHierarchy(const std::vector<SceneNode>& nodes)
{
    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        const uint32_t parent = nodes[index].parent;
        if (parent != kInvalidSceneNodeIndex &&
            (parent >= nodes.size() || parent == index))
        {
            throw std::invalid_argument("scene node parent is invalid");
        }
    }

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        std::vector<bool> visited(nodes.size(), false);
        uint32_t ancestor = index;
        while (ancestor != kInvalidSceneNodeIndex)
        {
            if (visited[ancestor])
            {
                throw std::invalid_argument("scene hierarchy contains a cycle");
            }
            visited[ancestor] = true;
            ancestor = nodes[ancestor].parent;
        }
    }
}

} // namespace

Scene::Scene(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void Scene::create(CreateInfo createInfo)
{
    validateHierarchy(createInfo.nodes);
    name_ = std::move(createInfo.name);
    nodes_ = std::move(createInfo.nodes);
}

void Scene::reset() noexcept
{
    name_.clear();
    nodes_.clear();
}

void Scene::setLocalTransform(
    uint32_t nodeIndex,
    const glm::mat4& transform)
{
    if (nodeIndex >= nodes_.size())
    {
        throw std::out_of_range("scene node index is out of range");
    }
    nodes_[nodeIndex].localTransform = transform;
}

} // namespace VkRenderer
