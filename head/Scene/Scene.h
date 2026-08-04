#pragma once

#include "Asset/AssetFwd.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace VkRenderer
{

inline constexpr uint32_t kInvalidSceneNodeIndex =
    std::numeric_limits<uint32_t>::max();

struct SceneNode
{
    std::string name;
    glm::mat4 localTransform{1.0f};
    uint32_t parent = kInvalidSceneNodeIndex;
    ModelAssetHandle model;
};

/// Runtime hierarchy that instances model assets without owning their data.
class Scene final
{
public:
    struct CreateInfo
    {
        std::string name;
        std::vector<SceneNode> nodes;
    };

    Scene() = default;
    explicit Scene(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;
    void setLocalTransform(uint32_t nodeIndex, const glm::mat4& transform);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<SceneNode>& nodes() const noexcept { return nodes_; }

private:
    std::string name_;
    std::vector<SceneNode> nodes_;
};

} // namespace VkRenderer
