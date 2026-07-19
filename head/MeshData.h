#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct GLBMesh;

namespace VkRenderer
{

struct Vertex
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec2 texCoord = glm::vec2(0.0f);
    glm::vec3 tangent = glm::vec3(0.0f);
    glm::vec2 texCoord2 = glm::vec2(0.0f);
    glm::vec4 color = glm::vec4(1.0f);

    std::array<int32_t, 4> boneIds = { -1, -1, -1, -1 };
    std::array<float, 4> boneWeights = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct SubmeshData
{
    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    int32_t materialIndex = -1;

    [[nodiscard]] bool indexed() const noexcept { return indexCount != 0; }
};

class MeshData final
{
public:
    MeshData() = default;

    [[nodiscard]] static MeshData fromGLBMesh(const GLBMesh& mesh);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<Vertex>& vertices() const noexcept { return vertices_; }
    [[nodiscard]] const std::vector<uint32_t>& indices() const noexcept { return indices_; }
    [[nodiscard]] const std::vector<SubmeshData>& submeshes() const noexcept { return submeshes_; }
    [[nodiscard]] bool empty() const noexcept { return vertices_.empty() || submeshes_.empty(); }

private:
    std::string name_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<SubmeshData> submeshes_;
};

} // namespace VkRenderer
