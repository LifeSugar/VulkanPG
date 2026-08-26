#pragma once

#include "asset/AssetManager.hpp"
#include "render/Camera.hpp"
#include "content/DemoContent.hpp"
#include "ImGuiLayer.hpp"
#include "texture/TextureImportRegistry.hpp"
#include "vulkan/RenderAssetCache.hpp"
#include "scene/Scene.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanApplicationGuiRenderBridge.hpp"
#include "vulkan/VulkanRenderer.hpp"
#include "vulkan/Window.hpp"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <future>
#include <optional>
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
        DemoContentLoader::CreateInfo demoContent{
            "ABeautifulGame_extracted/ABeautifulGame.gltf",
            "shaders/triangle.vert.spv",
            "shaders/triangle.frag.spv",
            "shaders/present.vert.spv",
            "shaders/present.frag.spv",
            "assets",
            DemoContentLoader::TextureImportPolicy{
                KtxPayloadEncoding::Uastc,
                true,
                TextureFormat::BC7UNorm,
                TextureFormat::BC7UNorm,
                TextureFormat::BC5UNorm,
                true}};
    };

    ~App();

    void setPreferIntegratedGPU(bool enabled);

    void run();
    void run(const RunConfig& config, ApplicationGui& gui);
private:
    struct PreparedTextureReimport
    {
        TextureReimportRequest request;
        TextureImportRecord record;
        std::filesystem::path stagedPath;
        TextureAsset replacementAsset;
        std::string error;
    };

#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

    Window window;
    VulkanContext vulkanContext;
    AssetManager assetManager;
    TextureImportRegistry textureImports;
    DemoContent demoContent;
    Scene scene;
    RenderAssetCache renderAssets;
    VulkanRenderer renderer;
    // Must be destroyed before the renderer, device, and GLFW window.
    ImGuiLayer imguiLayer;
    // Must release ImGui descriptors before ImGuiLayer is destroyed.
    VulkanApplicationGuiRenderBridge guiRenderBridge;

    Camera camera;
    static constexpr uint32_t kMaxFramesInFlight = 2;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    std::deque<TextureReimportRequest> pendingTextureReimports_;
    std::future<PreparedTextureReimport> textureReimportFuture_;
    TextureAssetHandle activeTextureReimport_;
    std::filesystem::path activeTextureReimportStagedPath_;
    double lastFramebufferResizeTime = 0.0;
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow(const RunConfig& config, bool visible = true);
    void initVulkan(const RunConfig& config);
    void initImGui(const RunConfig& config);
    void mainLoop(ApplicationGui& gui);
    void cleanup();
    void drawGui(ApplicationGui& gui);
    void processPendingTextureReimport();
    void discardTextureReimport() noexcept;

private:
    void setupCamera();

    void recreateSwapChain(ApplicationGui& gui);
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    [[nodiscard]] RenderFrame makeRenderFrame();

    friend class Test::AppSmokeTests;
};

} // namespace VkRenderer
