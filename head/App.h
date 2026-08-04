#pragma once

#include "Asset/AssetManager.h"
#include "Camera.h"
#include "RenderAssetCache.h"
#include "Scene/Scene.h"
#include "VulkanContext.h"
#include "VulkanRenderer.h"
#include "Window.h"

#include <cstdint>
#include <string>

namespace VkRenderer
{

class App
{
public:
    ~App();

    void setPreferIntegratedGPU(bool enabled);

    void run();
    /// Runs the CPU asset-import path without creating a window or Vulkan objects.
    void runAssetImportTest();
    /// Runs a small hidden-window Vulkan render smoke test.
    void runRenderTest();

private:
    static const uint32_t kWindowWidth = 1280;
    static const uint32_t kWindowHeight = 720;

#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

    Window window;
    VulkanContext vulkanContext;
    AssetManager assetManager;
    TextureAssetHandle demoTextureAsset;
    ShaderAssetHandle pbrVertexShaderAsset;
    ShaderAssetHandle pbrFragmentShaderAsset;
    MaterialTemplateAssetHandle demoMaterialTemplateAsset;
    MaterialAssetHandle demoMaterialAsset;
    ModelAssetHandle demoModelAsset;
    Scene scene;
    RenderAssetCache renderAssets;
    VulkanRenderer renderer;

    Camera camera;
    std::string modelPath = "Assets/Models/ABeautifulGame.glb";

    static constexpr uint32_t kMaxFramesInFlight = 2;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    double lastFramebufferResizeTime = 0.0;
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow(bool visible = true);
    void initVulkan();
    void mainLoop();
    void cleanup();

private:
    void setupCamera();

    void recreateSwapChain();
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    [[nodiscard]] RenderFrame makeRenderFrame();
    void createDemoAssets();

    static std::string resolveAssetPath(const std::string& relativePath);
    [[nodiscard]] GraphicsPipeline::CreateInfo makeGraphicsPipelineCreateInfo() const;

};

} // namespace VkRenderer
