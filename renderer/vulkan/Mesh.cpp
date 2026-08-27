#include "vulkan/Mesh.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace rubia::rhi::vulkan
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
    const asset::MeshAsset& asset)
{
    create(uploadContext, asset);
}

void Mesh::create(
    UploadContext& uploadContext,
    const asset::MeshAsset& asset)
{
    if (asset.empty())
    {
        throw std::invalid_argument("cannot create a Mesh from an empty mesh asset");
    }

    const VkDeviceSize vertexSize = checkedBufferSize(
        asset.vertices().size(),
        sizeof(asset::Vertex),
        "mesh vertex data");

    std::vector<asset::SubmeshData> newSubmeshes = asset.submeshes();
    const math::Aabb newLocalBounds = asset.localBounds();
    Buffer newVertexBuffer = uploadContext.uploadBuffer(
        asset.vertices().data(),
        vertexSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    Buffer newIndexBuffer;
    if (!asset.indices().empty())
    {
        const VkDeviceSize indexSize = checkedBufferSize(
            asset.indices().size(),
            sizeof(uint32_t),
            "mesh index data");
        newIndexBuffer = uploadContext.uploadBuffer(
            asset.indices().data(),
            indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    reset();
    vertexBuffer_ = std::move(newVertexBuffer);
    indexBuffer_ = std::move(newIndexBuffer);
    submeshes_ = std::move(newSubmeshes);
    localBounds_ = newLocalBounds;
}

void Mesh::reset() noexcept
{
    submeshes_.clear();
    localBounds_ = {};
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
    if (indexBuffer_)
    {
        vkCmdBindIndexBuffer(
            commandBuffer,
            indexBuffer_.get(),
            0,
            VK_INDEX_TYPE_UINT32);
    }
}

} // namespace rubia::rhi::vulkan
