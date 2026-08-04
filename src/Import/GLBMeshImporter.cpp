#include "Import/GLBMeshImporter.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

uint32_t checkedSize(std::size_t size, const char* description)
{
    if (size > std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error(
            std::string(description) + " exceeds the uint32_t range");
    }
    return static_cast<uint32_t>(size);
}

Vertex convertVertex(const GLBVertex& source)
{
    Vertex destination{};
    destination.position = source.position;
    destination.normal = source.normal;
    destination.texCoord = source.texCoord;
    destination.tangent = source.tangent;
    destination.texCoord2 = source.texCoord2;
    destination.color = source.color;
    destination.boneIds = source.boneIds;
    destination.boneWeights = source.boneWeights;
    return destination;
}

MaterialAssetHandle resolveMaterial(
    int sourceIndex,
    const GLBMeshImporter::CreateInfo& createInfo)
{
    if (sourceIndex < 0)
    {
        return createInfo.fallbackMaterial;
    }

    if (createInfo.materials != nullptr &&
        static_cast<std::size_t>(sourceIndex) <
            createInfo.materials->size())
    {
        const MaterialAssetHandle material =
            (*createInfo.materials)[static_cast<std::size_t>(sourceIndex)];
        if (material)
        {
            return material;
        }
    }

    if (createInfo.fallbackMaterial)
    {
        return createInfo.fallbackMaterial;
    }

    throw std::invalid_argument(
        "GLB primitive references a material outside the imported material table");
}

MeshAsset::CreateInfo convertMesh(
    const GLBMesh& source,
    const GLBMeshImporter::CreateInfo& createInfo)
{
    MeshAsset::CreateInfo destination{};
    destination.name = source.name;

    std::size_t vertexCount = 0;
    std::size_t indexCount = 0;
    for (const GLBPrimitive& primitive : source.primitives)
    {
        vertexCount += primitive.vertices.size();
        indexCount += primitive.indices.size();
    }
    destination.vertices.reserve(vertexCount);
    destination.indices.reserve(indexCount);
    destination.submeshes.reserve(source.primitives.size());

    for (const GLBPrimitive& primitive : source.primitives)
    {
        if (primitive.vertices.empty())
        {
            continue;
        }

        SubmeshData submesh{};
        submesh.firstVertex = checkedSize(
            destination.vertices.size(),
            "imported mesh vertex offset");
        submesh.vertexCount = checkedSize(
            primitive.vertices.size(),
            "imported submesh vertex count");
        submesh.firstIndex = checkedSize(
            destination.indices.size(),
            "imported mesh index offset");
        submesh.indexCount = checkedSize(
            primitive.indices.size(),
            "imported submesh index count");
        submesh.material = resolveMaterial(
            primitive.materialIndex,
            createInfo);

        for (const GLBVertex& vertex : primitive.vertices)
        {
            destination.vertices.push_back(convertVertex(vertex));
        }

        for (uint32_t index : primitive.indices)
        {
            if (index >= submesh.vertexCount)
            {
                throw std::invalid_argument(
                    "GLB primitive contains an out-of-range vertex index");
            }
            if (index >
                std::numeric_limits<uint32_t>::max() - submesh.firstVertex)
            {
                throw std::overflow_error(
                    "combined GLB mesh index exceeds the uint32_t range");
            }
            destination.indices.push_back(submesh.firstVertex + index);
        }

        destination.submeshes.push_back(submesh);
    }
    return destination;
}

} // namespace

std::vector<MeshAssetHandle> GLBMeshImporter::import(
    const std::vector<GLBMesh>& source,
    const CreateInfo& createInfo) const
{
    if (createInfo.assets == nullptr)
    {
        throw std::invalid_argument(
            "GLBMeshImporter requires an AssetManager");
    }
    if (createInfo.fallbackMaterial &&
        !createInfo.assets->contains(createInfo.fallbackMaterial))
    {
        throw std::invalid_argument(
            "GLBMeshImporter fallback material is not owned by its AssetManager");
    }
    std::vector<MeshAssetHandle> result;
    result.reserve(source.size());
    for (const GLBMesh& mesh : source)
    {
        result.push_back(createInfo.assets->createMesh(
            convertMesh(mesh, createInfo)));
    }
    return result;
}

} // namespace VkRenderer
