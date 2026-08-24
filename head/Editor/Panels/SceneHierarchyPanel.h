#pragma once

namespace VkRenderer
{

class Scene;
class AssetManager;
class EditorSelection;

/// Draws the SceneNode hierarchy without depending on Renderer state.
/// Referenced ModelAsset nodes are projected as a read-only subtree.
class SceneHierarchyPanel final
{
public:
    void draw(
        const Scene& scene,
        const AssetManager& assets,
        EditorSelection& selection,
        bool* open = nullptr);
};

} // namespace VkRenderer
