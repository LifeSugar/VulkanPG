#pragma once

#include "Asset/AssetManager.h"
#include "Camera.h"
#include "Content/DemoContent.h"
#include "ImGuiLayer.h"
#include "RenderAssetCache.h"
#include "Scene/Scene.h"
#include "VulkanContext.h"
#include "VulkanRenderer.h"
#include "Window.h"

#include <cstdint>
#include <string>

namespace VkRenderer
{

class ApplicationGui;
namespace Test
{
class AppSmokeTests;
}

class App
{
public:
    struct RunConfig
    {
        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
        std::string windowTitle = "Vulkan";
        bool enableDocking = true;
        std::string imguiIniFilename;
        VulkanRenderer::OutputMode outputMode =
            VulkanRenderer::OutputMode::Runtime;
    };

    ~App();

    void setPreferIntegratedGPU(bool enabled);

    void run();
    void run(const RunConfig& config, ApplicationGui& gui);
private:
#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

    Window window;
    VulkanContext vulkanContext;
    AssetManager assetManager;
    DemoContent demoContent;
    Scene scene;
    RenderAssetCache renderAssets;
    VulkanRenderer renderer;
    // Must be destroyed before the renderer, device, and GLFW window.
    ImGuiLayer imguiLayer;

    Camera camera;
    static constexpr uint32_t kMaxFramesInFlight = 2;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    double lastFramebufferResizeTime = 0.0;
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow(const RunConfig& config, bool visible = true);
    void initVulkan(const RunConfig& config);
    void initImGui(const RunConfig& config);
    void mainLoop(ApplicationGui& gui);
    void cleanup();
    void drawGui(ApplicationGui& gui);

private:
    void setupCamera();

    void recreateSwapChain(ApplicationGui& gui);
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    [[nodiscard]] RenderFrame makeRenderFrame();

    friend class Test::AppSmokeTests;
};

} // namespace VkRenderer
