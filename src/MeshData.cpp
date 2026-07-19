#include "MeshData.h"

#include "GLBTypes.h"

#include <limits>
#include <stdexcept>

namespace VkRenderer
{
namespace
{

uint32_t checkedSize(std::size_t size, const char* description)
{
    if (size > std::numeric_limits<uint32_t>::max())
    {
        throw std::overflow_error(std::string(description) + " exceeds the uint32_t range");
    }
    return static_cast<uint32_t>(size);
}

} // namespace

MeshData MeshData::fromGLBMesh(const GLBMesh& mesh)
{
    MeshData result;
    result.name_ = mesh.name;

    std::size_t totalVertexCount = 0;
    std::size_t totalIndexCount = 0;
    for (const GLBPrimitive& primitive : mesh.primitives)
    {
        totalVertexCount += primitive.vertices.size();
        totalIndexCount += primitive.indices.size();
    }

    result.vertices_.reserve(totalVertexCount);
    result.indices_.reserve(totalIndexCount);
    result.submeshes_.reserve(mesh.primitives.size());

    for (const GLBPrimitive& primitive : mesh.primitives)
    {
        if (primitive.vertices.empty())
        {
            continue;
        }

        SubmeshData submesh{};
        submesh.firstVertex = checkedSize(result.vertices_.size(), "mesh vertex offset");
        submesh.vertexCount = checkedSize(primitive.vertices.size(), "submesh vertex count");
        submesh.firstIndex = checkedSize(result.indices_.size(), "mesh index offset");
        submesh.indexCount = checkedSize(primitive.indices.size(), "submesh index count");
        submesh.materialIndex = primitive.materialIndex;

        result.vertices_.insert(
            result.vertices_.end(),
            primitive.vertices.begin(),
            primitive.vertices.end());

        for (uint32_t index : primitive.indices)
        {
            if (index >= submesh.vertexCount)
            {
                throw std::runtime_error("GLB primitive contains an out-of-range vertex index");
            }
            if (index > std::numeric_limits<uint32_t>::max() - submesh.firstVertex)
            {
                throw std::overflow_error("combined mesh index exceeds the uint32_t range");
            }
            result.indices_.push_back(submesh.firstVertex + index);
        }

        result.submeshes_.push_back(submesh);
    }

    return result;
}

} // namespace VkRenderer
