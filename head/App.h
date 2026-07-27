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
#include "Window.h"

namespace VkRenderer
{

class App
{
public:
    ~App();

    void setPreferIntegratedGPU(bool enabled);

    void run();

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
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

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
