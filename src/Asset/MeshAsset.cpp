#include "Asset/MeshAsset.h"

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

void validate(const MeshAsset::CreateInfo& createInfo)
{
    if (createInfo.vertices.empty() || createInfo.submeshes.empty())
    {
        throw std::invalid_argument(
            "mesh asset requires vertices and at least one submesh");
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
    name_ = std::move(createInfo.name);
    vertices_ = std::move(createInfo.vertices);
    indices_ = std::move(createInfo.indices);
    submeshes_ = std::move(createInfo.submeshes);
}

void MeshAsset::reset() noexcept
{
    name_.clear();
    vertices_.clear();
    indices_.clear();
    submeshes_.clear();
}

} // namespace VkRenderer
