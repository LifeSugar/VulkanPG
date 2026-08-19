#include "Asset/MeshAsset.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{
namespace
{

bool rangeFits(uint32_t first, uint32_t count, std::size_t capacity)
{
    return first <= capacity && count <= capacity - first;
}

bool finitePosition(const glm::vec3& position)
{
    return std::isfinite(position.x) &&
        std::isfinite(position.y) &&
        std::isfinite(position.z);
}

Aabb calculateLocalBounds(const std::vector<Vertex>& vertices)
{
    Aabb bounds{};
    bounds.minimum = vertices.front().position;
    bounds.maximum = vertices.front().position;
    for (const Vertex& vertex : vertices)
    {
        bounds.minimum = glm::min(bounds.minimum, vertex.position);
        bounds.maximum = glm::max(bounds.maximum, vertex.position);
    }
    return bounds;
}

void validate(const MeshAsset::CreateInfo& createInfo)
{
    if (createInfo.vertices.empty() || createInfo.submeshes.empty())
    {
        throw std::invalid_argument(
            "mesh asset requires vertices and at least one submesh");
    }

    if (std::any_of(
            createInfo.vertices.begin(),
            createInfo.vertices.end(),
            [](const Vertex& vertex)
            {
                return !finitePosition(vertex.position);
            }))
    {
        throw std::invalid_argument(
            "mesh asset vertex positions must be finite");
    }

    for (const SubmeshData& submesh : createInfo.submeshes)
    {
        if (submesh.indexed())
        {
            if (!rangeFits(
                    submesh.firstIndex,
                    submesh.indexCount,
                    createInfo.indices.size()))
            {
                throw std::invalid_argument(
                    "indexed submesh range exceeds the mesh index data");
            }
            for (uint32_t offset = 0; offset < submesh.indexCount; ++offset)
            {
                const uint32_t index =
                    createInfo.indices[submesh.firstIndex + offset];
                if (index >= createInfo.vertices.size())
                {
                    throw std::invalid_argument(
                        "mesh index references a vertex outside the mesh");
                }
            }
        }
        else if (submesh.vertexCount == 0 ||
                 !rangeFits(
                     submesh.firstVertex,
                     submesh.vertexCount,
                     createInfo.vertices.size()))
        {
            throw std::invalid_argument(
                "non-indexed submesh range exceeds the mesh vertex data");
        }
    }
}

} // namespace

MeshAsset::MeshAsset(CreateInfo createInfo)
{
    create(std::move(createInfo));
}

void MeshAsset::create(CreateInfo createInfo)
{
    validate(createInfo);
    const Aabb localBounds = calculateLocalBounds(createInfo.vertices);
    name_ = std::move(createInfo.name);
    vertices_ = std::move(createInfo.vertices);
    indices_ = std::move(createInfo.indices);
    submeshes_ = std::move(createInfo.submeshes);
    localBounds_ = localBounds;
}

void MeshAsset::reset() noexcept
{
    name_.clear();
    vertices_.clear();
    indices_.clear();
    submeshes_.clear();
    localBounds_ = {};
}

} // namespace VkRenderer
