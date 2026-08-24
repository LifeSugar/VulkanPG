#pragma once

#include "ApplicationGui.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace VkRenderer
{

/// Editor-owned ImGui business layer. It does not own runtime or Vulkan state.
class EditorLayer final : public ApplicationGui
{
public:
    void attach(const ApplicationGuiContext& context) override;
    void detach() noexcept override;
    [[nodiscard]] ApplicationGuiFrameOutput draw(
        const ApplicationGuiContext& context) override;

private:
    void drawDockSpace();
    void drawSceneHierarchy(const ApplicationGuiContext& context);
    void drawInspector(const ApplicationGuiContext& context);
    [[nodiscard]] std::optional<float> drawSceneViewport(
        const ApplicationGuiContext& context);
    void drawRendererStats(const ApplicationGuiContext& context);
    void registerViewportTextures(const VulkanRenderer& renderer);
    void releaseViewportTextures() noexcept;
    void refreshViewportTexturesIfNeeded(const VulkanRenderer& renderer);

    uint32_t selectedSceneNode_ = UINT32_MAX;
    std::vector<VkDescriptorSet> viewportTextures_;
    uint64_t viewportTextureRevision_ = 0;
    bool showSceneHierarchy_ = true;
    bool showInspector_ = true;
    bool showSceneViewport_ = true;
    bool showRendererStats_ = true;
};

} // namespace VkRenderer
