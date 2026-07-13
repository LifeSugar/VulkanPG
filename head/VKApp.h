#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <GLBTypes.h>
#include "Camera.h"
#include "GLBLoader.h"
#include "VulkanBuffer.h"
#include "VulkanSwapchain.h"

class VulkanApp
{
public:
    void setPreferIntegratedGPU(bool enabled);

    void run();

private:
    static const uint32_t kWindowWidth = 1280;
    static const uint32_t kWindowHeight = 720;

    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    GLFWwindow *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VulkanSwapchain swapChain;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicPipeline = VK_NULL_HANDLE;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    std::vector<VulkanBuffer> uniformBuffers;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model{1.0f};
        alignas(16) glm::mat4 view{1.0f};
        alignas(16) glm::mat4 proj{1.0f};
    };

    GLBLoader loader;
    std::unique_ptr<GLBModel> model;
    std::string modelPath = "Assets/Models/Suzanne.glb";
    uint32_t indexCount = 0;
    Camera camera;

    static constexpr uint32_t kMaxFramesInFlight = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame = 0;
    bool preferIntegratedGpu = false;
    bool swapChainRecreationRequested = false;
    double lastFramebufferResizeTime = 0.0;
    static constexpr double kSwapChainResizeDebounceSeconds = 0.15;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

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

    static const char *PhysicalDeviceTypeToString(VkPhysicalDeviceType type);
    static std::string queueFamilyIndicesString(VkQueueFlags flags);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice candidate) const;
    bool isDeviceSuitable(VkPhysicalDevice candidate) const;
    void pickPhysicalDevice();
    void createLogicalDevice();

    VulkanSwapchain makeSwapChain(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE) const;
    void cleanupSwapChainDependents();
    void cleanupSwapChain();
    void recreateSwapChain();
    void requestSwapChainRecreation();
    bool isSwapChainRecreationDue() const;

    void createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage &image,
        VkDeviceMemory &imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkFormat findSupportedFormat(
        const std::vector<VkFormat> &candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void createVertexBuffer(const GLBPrimitive& primitive, VulkanBuffer& targetBuffer);
    void createIndexBuffer(const GLBPrimitive& primitive, VulkanBuffer& targetBuffer);
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t imageIndex);
    void createDescriptorPool();
    void createDescriptorSets();
    void loadModel();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void createDepthResources();
    void createRenderPass();
    static std::string resolveAssetPath(const std::string& relativePath);
    static std::vector<char> readFile(const std::string &filename);
    VkShaderModule createShaderModule(const std::vector<char> &code) const;
    void createDescriptorSetLayout();
    void createGraphicsPipeline();

    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void drawFrame();

        


};
