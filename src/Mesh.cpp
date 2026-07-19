#include "Mesh.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace VkRenderer
{
namespace
{

VkDeviceSize checkedBufferSize(
    std::size_t elementCount,
    std::size_t elementSize,
    const char* description)
{
    if (elementCount == 0 ||
        elementCount > std::numeric_limits<VkDeviceSize>::max() / elementSize)
    {
        throw std::overflow_error(std::string(description) + " has an invalid byte size");
    }
    return static_cast<VkDeviceSize>(elementCount * elementSize);
}

} // namespace

Mesh::Mesh(
    UploadContext& uploadContext,
    const MeshData& data)
{
    create(uploadContext, data);
}

void Mesh::create(
    UploadContext& uploadContext,
    const MeshData& data)
{
    if (data.empty() || data.indices().empty())
    {
        throw std::invalid_argument("cannot create a Mesh from empty or non-indexed mesh data");
    }

    const VkDeviceSize vertexSize = checkedBufferSize(
        data.vertices().size(),
        sizeof(Vertex),
        "mesh vertex data");
    const VkDeviceSize indexSize = checkedBufferSize(
        data.indices().size(),
        sizeof(uint32_t),
        "mesh index data");

    std::vector<SubmeshData> newSubmeshes = data.submeshes();
    Buffer newVertexBuffer = uploadContext.uploadBuffer(
        data.vertices().data(),
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    Buffer newIndexBuffer = uploadContext.uploadBuffer(
        data.indices().data(),
        indexSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    reset();
    vertexBuffer_ = std::move(newVertexBuffer);
    indexBuffer_ = std::move(newIndexBuffer);
    submeshes_ = std::move(newSubmeshes);
}

void Mesh::reset() noexcept
{
    submeshes_.clear();
    indexBuffer_.reset();
    vertexBuffer_.reset();
}

void Mesh::bind(VkCommandBuffer commandBuffer) const
{
    if (commandBuffer == VK_NULL_HANDLE || !*this)
    {
        throw std::invalid_argument("cannot bind an invalid Mesh or command buffer");
    }

    const VkBuffer vertexBuffer = vertexBuffer_.get();
    constexpr VkDeviceSize kOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &kOffset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_.get(), 0, VK_INDEX_TYPE_UINT32);
}

} // namespace VkRenderer
