#pragma once

#include "texture/TextureImportRegistry.hpp"

#include <optional>
#include <vector>

namespace VkRenderer
{

class AssetManager;
class ApplicationGuiRenderBridge;
class Scene;

/// Non-owning services exposed to one application GUI frame.
struct ApplicationGuiContext
{
    AssetManager& assets;
    Scene& scene;
    ApplicationGuiRenderBridge& render;
    const TextureImportRegistry* textureImports = nullptr;
};

/// Per-frame GUI decisions consumed before building the scene RenderFrame.
struct ApplicationGuiFrameOutput
{
    std::optional<float> sceneAspectRatio;
    std::vector<TextureReimportRequest> textureReimports;
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
