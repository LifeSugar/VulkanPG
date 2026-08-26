#pragma once

#include "asset/AssetFwd.hpp"

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace VkRenderer
{

struct SceneNodeTarget final
{
    uint32_t nodeIndex = 0;
};

struct ModelNodeTarget final
{
    ModelAssetHandle model;
    uint32_t nodeIndex = 0;
    /// Identifies the SceneNode instance when selected from the Hierarchy.
    std::optional<uint32_t> sceneNodeIndex;
};

struct SubmeshTarget final
{
    MeshAssetHandle mesh;
    uint32_t submeshIndex = 0;
};

using InspectorTarget = std::variant<
    std::monostate,
    SceneNodeTarget,
    ModelAssetHandle,
    ModelNodeTarget,
    MeshAssetHandle,
    SubmeshTarget,
    MaterialAssetHandle,
    MaterialTemplateAssetHandle,
    TextureAssetHandle>;

/// Central Editor-only selection state. It never owns Scene or Asset data.
class EditorSelection final
{
public:
    /// Replaces the primary selection and starts a new navigation history.
    void select(InspectorTarget target);
    /// Follows an Inspector reference while retaining a Back destination.
    void navigateTo(InspectorTarget target);
    [[nodiscard]] bool canNavigateBack() const noexcept;
    void navigateBack();
    void clear() noexcept;

    [[nodiscard]] const InspectorTarget& target() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    InspectorTarget target_;
    std::vector<InspectorTarget> history_;
};

} // namespace VkRenderer
