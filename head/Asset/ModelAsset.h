#pragma once

#include "Asset/AssetFwd.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace VkRenderer
{

inline constexpr uint32_t kInvalidModelNodeIndex =
    std::numeric_limits<uint32_t>::max();

/// Source-independent node in a model hierarchy.
struct ModelNode
{
    std::string name;
    glm::mat4 localTransform{1.0f};
    uint32_t parent = kInvalidModelNodeIndex;
    std::vector<MeshAssetHandle> meshes;
};

/// Reusable hierarchy that references independently managed mesh assets.
class ModelAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        std::vector<ModelNode> nodes;
    };

    ModelAsset() = default;
    explicit ModelAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<ModelNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] explicit operator bool() const noexcept { return !nodes_.empty(); }

private:
    std::string name_;
    std::vector<ModelNode> nodes_;
};

} // namespace VkRenderer
