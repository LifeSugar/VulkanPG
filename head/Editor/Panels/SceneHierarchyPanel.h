#pragma once

#include <cstdint>
#include <limits>
#include <optional>

namespace VkRenderer
{

class Scene;
class AssetManager;

/// Draws the SceneNode hierarchy without depending on Renderer state.
/// Referenced ModelAsset nodes are projected as a read-only subtree.
class SceneHierarchyPanel final
{
public:
    void draw(
        const Scene& scene,
        const AssetManager& assets,
        bool* open = nullptr);

    [[nodiscard]] std::optional<uint32_t> selectedNodeIndex() const
        noexcept;
    void clearSelection() noexcept;

private:
    uint32_t selectedNodeIndex_ = std::numeric_limits<uint32_t>::max();
};

} // namespace VkRenderer
