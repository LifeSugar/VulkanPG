#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <GLBTypes.h>
#include "Camera.h"
#include "GLBLoader.h"
#include "Device.h"
#include "FrameDataResources.h"
#include "FrameResources.h"
#include "Mesh.h"
#include "Swapchain.h"

namespace VkRenderer
{

class App
{
public:
    void setPreferIntegratedGPU(bool enabled);

    void run();

private:
    static const uint32_t kWindowWidth = 1280;
    static const uint32_t kWindowHeight = 720;

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    GLFWwindow *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    Device device;
    Swapchain swapChain;
    std::vector<SwapchainFrame> swapchainFrames;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicPipeline = VK_NULL_HANDLE;

    Mesh mesh;
    FrameDataResources frameDataResources;

    GLBLoader loader;
    std::unique_ptr<GLBModel> model;
    std::string modelPath = "Assets/Models/Suzanne.glb";
    Camera camera;
    uint64_t stagedCameraRevision = 0;

    static constexpr uint32_t kMaxFramesInFlight = 2;
    std::vector<FrameInFlight> framesInFlight;
    uint32_t currentFrame = 0;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    double lastFramebufferResizeTime = 0.0;
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    bool checkValidationLayerSupport() const;
    std::vector<const char *> getRequiredExtensions() const;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);
    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance inInstance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator);
    void setupDebugMessenger();

    void createInstance();
    void createSurface();
    void setupCamera();

    Swapchain makeSwapChain(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE) const;
    void cleanupSwapChainDependents();
    void cleanupSwapChain();
    void recreateSwapChain();
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    VkFormat findSupportedFormat(
        const std::vector<VkFormat> &candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
    void updateFrameData(uint32_t frameIndex);
    [[nodiscard]] MeshData loadModel();

    void createDepthResources();
    void createRenderPass();
    static std::string resolveAssetPath(const std::string& relativePath);
    static std::vector<char> readFile(const std::string &filename);
    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    void createGraphicsPipeline();

    void createFramebuffers();
    void createCommandPools();
    void createCommandBuffers();
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        VkDescriptorSet descriptorSet);
    void createSyncObjects();
    void createSwapchainFrameSyncObjects();

    void drawFrame();

        


};

} // namespace VkRenderer
