#include "App.h"

namespace VkRenderer
{

void App::setPreferIntegratedGPU(bool enabled)
{
    preferIntegratedGpu = enabled;
}

void App::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void App::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void App::framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr)
    {
        app->requestSwapChainRecreation();
    }
}

void App::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    device.create(instance, surface, preferIntegratedGpu);
    swapChain = makeSwapChain();
    setupCamera();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    const MeshData meshData = loadModel();
    UploadContext uploadContext(device, commandPool);
    mesh.create(uploadContext, meshData);
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void App::cleanup()
{
    for (size_t i = 0; i < kMaxFramesInFlight; ++i)
    {
        
        vkDestroySemaphore(device.get(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.get(), inFlightFences[i], nullptr);
    }
    for (size_t i = 0; i < swapChain.imageCount(); ++i)
    {
        vkDestroySemaphore(device.get(), renderFinishedSemaphores[i], nullptr);
    }
    cleanupSwapChain();

    commandPool.reset();

    mesh.reset();

    vkDestroyDescriptorSetLayout(device.get(), descriptorSetLayout, nullptr);
    device.reset();
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::setupCamera()
{
    camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    const VkExtent2D extent = swapChain.extent();
    camera.setAspect(static_cast<float>(extent.width) / static_cast<float>(extent.height));
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (swapChainRecreationRequested)
        {
            if (!isSwapChainRecreationDue())
            {
                // During an interactive resize, avoid repeatedly destroying and recreating GPU resources.
                glfwWaitEventsTimeout(0.016);
                continue;
            }

            recreateSwapChain();
        }

        drawFrame();
    }
    device.waitIdle();
}

} // namespace VkRenderer
