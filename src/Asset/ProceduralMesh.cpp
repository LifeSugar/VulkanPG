#include "Asset/ProceduralMesh.h"

namespace VkRenderer
{
namespace
{

Vertex makeVertex(
    glm::vec3 position,
    glm::vec3 normal,
    glm::vec4 color,
    glm::vec2 texCoord)
{
    Vertex vertex{};
    vertex.position = position;
    vertex.normal = normal;
    vertex.color = color;
    vertex.texCoord = texCoord;
    return vertex;
}

} // namespace

MeshAsset::CreateInfo makeCubeMeshCreateInfo()
{
    MeshAsset::CreateInfo createInfo{};
    createInfo.name = "Procedural Cube";

    constexpr float n = -1.0f;
    constexpr float p = 1.0f;
    const glm::vec4 white(1.0f);

    createInfo.vertices = {
        makeVertex({n, n, p}, {0, 0, 1}, white, {0, 0}),
        makeVertex({p, n, p}, {0, 0, 1}, white, {1, 0}),
        makeVertex({p, p, p}, {0, 0, 1}, white, {1, 1}),
        makeVertex({n, p, p}, {0, 0, 1}, white, {0, 1}),

        makeVertex({p, n, n}, {0, 0, -1}, white, {0, 0}),
        makeVertex({n, n, n}, {0, 0, -1}, white, {1, 0}),
        makeVertex({n, p, n}, {0, 0, -1}, white, {1, 1}),
        makeVertex({p, p, n}, {0, 0, -1}, white, {0, 1}),

        makeVertex({n, n, n}, {-1, 0, 0}, white, {0, 0}),
        makeVertex({n, n, p}, {-1, 0, 0}, white, {1, 0}),
        makeVertex({n, p, p}, {-1, 0, 0}, white, {1, 1}),
        makeVertex({n, p, n}, {-1, 0, 0}, white, {0, 1}),

        makeVertex({p, n, p}, {1, 0, 0}, white, {0, 0}),
        makeVertex({p, n, n}, {1, 0, 0}, white, {1, 0}),
        makeVertex({p, p, n}, {1, 0, 0}, white, {1, 1}),
        makeVertex({p, p, p}, {1, 0, 0}, white, {0, 1}),

        makeVertex({n, p, p}, {0, 1, 0}, white, {0, 0}),
        makeVertex({p, p, p}, {0, 1, 0}, white, {1, 0}),
        makeVertex({p, p, n}, {0, 1, 0}, white, {1, 1}),
        makeVertex({n, p, n}, {0, 1, 0}, white, {0, 1}),

        makeVertex({n, n, n}, {0, -1, 0}, white, {0, 0}),
        makeVertex({p, n, n}, {0, -1, 0}, white, {1, 0}),
        makeVertex({p, n, p}, {0, -1, 0}, white, {1, 1}),
        makeVertex({n, n, p}, {0, -1, 0}, white, {0, 1})
    };

    createInfo.indices = {
         0,  1,  2,  2,  3,  0,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    SubmeshData submesh{};
    submesh.vertexCount = static_cast<uint32_t>(createInfo.vertices.size());
    submesh.indexCount = static_cast<uint32_t>(createInfo.indices.size());
    createInfo.submeshes.push_back(submesh);
    return createInfo;
}

} // namespace VkRenderer
