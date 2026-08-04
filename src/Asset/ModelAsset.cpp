#include "Asset/ModelAsset.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

void validateHierarchy(const std::vector<ModelNode>& nodes)
{
    if (nodes.empty())
    {
        throw std::invalid_argument("model asset requires at least one node");
    }

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        const uint32_t parent = nodes[index].parent;
        if (parent != kInvalidModelNodeIndex &&
            (parent >= nodes.size() || parent == index))
        {
            throw std::invalid_argument("model node parent is invalid");
        }
    }

    for (uint32_t index = 0;
         index < static_cast<uint32_t>(nodes.size());
         ++index)
    {
        std::vector<bool> visited(nodes.size(), false);
        uint32_t ancestor = index;
        while (ancestor != kInvalidModelNodeIndex)
        {
            if (visited[ancestor])
            {
                throw std::invalid_argument("model node hierarchy contains a cycle");
            }
            visited[ancestor] = true;
            ancestor = nodes[ancestor].parent;
        }
    }
}

} // namespace

ModelAsset::ModelAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void ModelAsset::create(CreateInfo createInfo)
{
    validateHierarchy(createInfo.nodes);
    name_ = std::move(createInfo.name);
    nodes_ = std::move(createInfo.nodes);
}

void ModelAsset::reset() noexcept
{
    name_.clear();
    nodes_.clear();
}

} // namespace VkRenderer
