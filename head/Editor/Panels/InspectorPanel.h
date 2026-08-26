#pragma once

#include "Editor/EditorSelection.h"
#include "Editor/Inspectors/MaterialInspector.h"
#include "Editor/Inspectors/ModelInspector.h"
#include "Editor/Inspectors/SceneNodeInspector.h"
#include "Editor/Inspectors/TextureInspector.h"

#include <vector>

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;
class Scene;

/// Owns the Inspector window and routes selections to type inspectors.
class InspectorPanel final
{
public:
    [[nodiscard]] std::vector<TextureReimportRequest> draw(
        const Scene& scene,
        const AssetManager& assets,
        ApplicationGuiRenderBridge& texturePreviews,
        const TextureImportRegistry* textureImports,
        EditorSelection& selection,
        bool* open = nullptr);

private:
    SceneNodeInspector sceneNodeInspector_;
    ModelInspector modelInspector_;
    MaterialInspector materialInspector_;
    TextureInspector textureInspector_;
};

} // namespace VkRenderer
