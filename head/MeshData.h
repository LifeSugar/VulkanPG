#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct GLBMesh;

namespace VkRenderer
{

/// Vertex attributes used by the renderer's mesh pipeline.
struct Vertex
{
    /// Object-space vertex position.
    glm::vec3 position = glm::vec3(0.0f);
    /// Object-space surface normal.
    glm::vec3 normal = glm::vec3(0.0f);
    /// Primary texture coordinates.
    glm::vec2 texCoord = glm::vec2(0.0f);
    /// Object-space tangent vector.
    glm::vec3 tangent = glm::vec3(0.0f);
    /// Secondary texture coordinates.
    glm::vec2 texCoord2 = glm::vec2(0.0f);
    /// Per-vertex color multiplier.
    glm::vec4 color = glm::vec4(1.0f);

    /// Indices of bones influencing this vertex.
    std::array<int32_t, 4> boneIds = { -1, -1, -1, -1 };
    /// Weights of the corresponding bone influences.
    std::array<float, 4> boneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
};

/// Draw range and material reference for one mesh primitive.
struct SubmeshData
{
    /// First vertex used by this submesh.
    uint32_t firstVertex = 0;
    /// Number of vertices in this submesh.
    uint32_t vertexCount = 0;
    /// First index used by this submesh.
    uint32_t firstIndex = 0;
    /// Number of indices in this submesh.
    uint32_t indexCount = 0;
    /// Source material index, or -1 when absent.
    int32_t materialIndex = -1;

    /// Returns whether this submesh uses indexed drawing.
    [[nodiscard]] bool indexed() const noexcept { return indexCount != 0; }
};

/// CPU-side mesh data ready for GPU upload.
class MeshData final
{
public:
    /// Creates an empty mesh-data container.
    MeshData() = default;

    /// Flattens a GLB mesh into contiguous vertex, index, and draw data.
    [[nodiscard]] static MeshData fromGLBMesh(const GLBMesh& mesh);

    /// Returns the source mesh name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// Returns the flattened vertex data.
    [[nodiscard]] const std::vector<Vertex>& vertices() const noexcept { return vertices_; }
    /// Returns the flattened index data.
    [[nodiscard]] const std::vector<uint32_t>& indices() const noexcept { return indices_; }
    /// Returns the draw ranges for all source primitives.
    [[nodiscard]] const std::vector<SubmeshData>& submeshes() const noexcept { return submeshes_; }
    /// Returns whether the mesh lacks vertices or draw ranges.
    [[nodiscard]] bool empty() const noexcept { return vertices_.empty() || submeshes_.empty(); }

private:
    /// Name copied from the source mesh.
    std::string name_;
    /// Contiguous vertices from all source primitives.
    std::vector<Vertex> vertices_;
    /// Contiguous indices adjusted for the flattened vertex array.
    std::vector<uint32_t> indices_;
    /// Draw ranges into the flattened vertex and index arrays.
    std::vector<SubmeshData> submeshes_;
};

} // namespace VkRenderer
