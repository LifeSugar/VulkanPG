#pragma once

#include "ApplicationGui.h"
#include "Editor/EditorSelection.h"
#include "Editor/Panels/InspectorPanel.h"
#include "Editor/Panels/SceneHierarchyPanel.h"

#include <cstdint>

namespace VkRenderer
{

/// Editor-owned ImGui business layer. It does not own runtime or Vulkan state.
class EditorLayer final : public ApplicationGui
{
public:
    [[nodiscard]] ApplicationGuiFrameOutput draw(
        const ApplicationGuiContext& context) override;

private:
    void drawDockSpace();
    [[nodiscard]] std::optional<float> drawSceneViewport(
        const ApplicationGuiContext& context);
    void drawRendererStats(const ApplicationGuiContext& context);

    EditorSelection selection_;
    SceneHierarchyPanel sceneHierarchyPanel_;
    InspectorPanel inspectorPanel_;
    uint32_t sceneViewportWidth_ = 0;
    uint32_t sceneViewportHeight_ = 0;
    bool showSceneHierarchy_ = true;
    bool showInspector_ = true;
    bool showSceneViewport_ = true;
    bool showRendererStats_ = true;
};

} // namespace VkRenderer
