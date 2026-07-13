#include "VKApp.h"

void VulkanApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int, int)
{
    auto* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr)
    {
        app->requestSwapChainRecreation();
    }
}

void VulkanApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    setupCamera();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    loadModel();
    GLBPrimitive& primitive = model->meshes[0].primitives[0];
    createVertexBuffer(primitive, vertexBuffer, vertexBufferMemory);
    createIndexBuffer(primitive, indexBuffer, indexBufferMemory);
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanApp::cleanup()
{
    for (size_t i = 0; i < kMaxFramesInFlight; ++i)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    cleanupSwapChain();

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApp::setupCamera()
{
    camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.setAspect(static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height));
}

void VulkanApp::mainLoop()
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
    vkDeviceWaitIdle(device);
}

