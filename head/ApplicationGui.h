#pragma once

#include <optional>

namespace VkRenderer
{

class AssetManager;
class RenderAssetCache;
class Scene;
class VulkanRenderer;

/// Non-owning services exposed to one application GUI frame.
struct ApplicationGuiContext
{
    AssetManager& assets;
    Scene& scene;
    RenderAssetCache& renderAssets;
    VulkanRenderer& renderer;
};

/// Per-frame GUI decisions consumed before building the scene RenderFrame.
struct ApplicationGuiFrameOutput
{
    std::optional<float> sceneAspectRatio;
};

/// UI business layer consumed by App without depending on Runtime or Editor UI.
class ApplicationGui
{
public:
    virtual ~ApplicationGui() = default;

    virtual void attach(const ApplicationGuiContext&) {}
    virtual void detach() noexcept {}
    [[nodiscard]] virtual ApplicationGuiFrameOutput draw(
        const ApplicationGuiContext& context) = 0;
};

} // namespace VkRenderer
