#pragma once

#include "Editor/EditorSelection.h"
#include "Editor/EditorTexturePreview.h"
#include "Editor/Panels/TransformPanel.h"

#include <optional>

namespace VkRenderer
{

class AssetManager;
class Scene;

/// Routes the current Editor selection to a read-only type-specific view.
class InspectorPanel final
{
public:
    void draw(
        const Scene& scene,
        const AssetManager& assets,
        EditorTexturePreviewProvider& texturePreviews,
        EditorSelection& selection,
        bool* open = nullptr);

private:
    void drawSceneNode(
        const Scene& scene,
        const AssetManager& assets,
        SceneNodeTarget target);
    void drawModelAsset(
        const AssetManager& assets,
        ModelAssetHandle target);
    void drawModelNode(
        const Scene& scene,
        const AssetManager& assets,
        ModelNodeTarget target);
    void drawMeshAsset(
        const AssetManager& assets,
        MeshAssetHandle target);
    void drawSubmesh(
        const AssetManager& assets,
        SubmeshTarget target);
    void drawMaterialAsset(
        const AssetManager& assets,
        EditorTexturePreviewProvider& texturePreviews,
        MaterialAssetHandle target);
    void drawMaterialTemplate(
        const AssetManager& assets,
        MaterialTemplateAssetHandle target);
    void drawTextureAsset(
        const AssetManager& assets,
        EditorTexturePreviewProvider& texturePreviews,
        TextureAssetHandle target);
    void requestNavigation(InspectorTarget target);

    std::optional<InspectorTarget> pendingNavigation_;
    TransformPanel transformPanel_;
};

} // namespace VkRenderer
