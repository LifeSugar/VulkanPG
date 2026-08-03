#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <GLBTypes.h>
#include "Camera.h"
#include "GLBLoader.h"
#include "Mesh.h"
#include "VulkanContext.h"
#include "VulkanRenderer.h"
#ifndef __ANDROID__
#include "Window.h"
#endif

namespace VkRenderer
{

class App
{
public:
    ~App();

    void setPreferIntegratedGPU(bool enabled);

    /// Desktop: full lifecycle (window init → Vulkan init → main loop → cleanup).
    void run();
    /// Android: initialize Vulkan with an externally managed surface.
    void initVulkan(VkSurfaceKHR surface, uint32_t width, uint32_t height);
    /// Android: render one frame (called from the Android event loop).
    void renderFrame();
    /// Android: directly resize swapchain with the given extent.
    void resizeSwapchain(VkExtent2D newExtent);
    /// Releases all resources. Safe to call from Android lifecycle.
    void cleanup();

    /// Mutable access to the Vulkan context (for Android two-phase init).
    VulkanContext& vulkanContext() { return vulkanContext_; }

private:
    static const uint32_t kWindowWidth = 1280;
    static const uint32_t kWindowHeight = 720;

#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

#ifdef __ANDROID__
    // On Android, VulkanContext holds the surface directly; Window is unused.
#else
    Window window;
#endif
    VulkanContext vulkanContext_;
    Mesh mesh;
    VulkanRenderer renderer;

    GLBLoader loader;
    std::unique_ptr<GLBModel> model;
    std::string modelPath = "Assets/Models/Suzanne.glb";
    Camera camera;

    static constexpr uint32_t kMaxFramesInFlight = 2;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    double lastFramebufferResizeTime = 0.0;
    VkExtent2D pendingResizeExtent_{};
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow();
    void initVulkan();
    /// Shared Vulkan initialization after context + renderer are created.
    void initVulkanCommon(VkExtent2D framebufferExtent);
    void mainLoop();

private:
    void setupCamera();

    void recreateSwapChain();
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    [[nodiscard]] RenderFrame makeRenderFrame();
    [[nodiscard]] MeshData loadModel();

    static std::string resolveAssetPath(const std::string& relativePath);
    [[nodiscard]] GraphicsPipeline::CreateInfo makeGraphicsPipelineCreateInfo() const;

};

} // namespace VkRenderer
