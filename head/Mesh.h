#pragma once

#include "Buffer.h"
#include "MeshData.h"
#include "UploadContext.h"

#include <vector>

namespace VkRenderer
{

class Mesh final
{
public:
    Mesh() = default;
    Mesh(
        UploadContext& uploadContext,
        const MeshData& data);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void create(
        UploadContext& uploadContext,
        const MeshData& data);
    void reset() noexcept;
    void bind(VkCommandBuffer commandBuffer) const;

    [[nodiscard]] VkBuffer vertexBuffer() const noexcept { return vertexBuffer_.get(); }
    [[nodiscard]] VkBuffer indexBuffer() const noexcept { return indexBuffer_.get(); }
    [[nodiscard]] const std::vector<SubmeshData>& submeshes() const noexcept { return submeshes_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return vertexBuffer_ && indexBuffer_ && !submeshes_.empty();
    }

private:
    Buffer vertexBuffer_;
    Buffer indexBuffer_;
    std::vector<SubmeshData> submeshes_;
};

} // namespace VkRenderer
