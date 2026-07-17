#include "VKApp.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

VulkanSwapchain VulkanApp::makeSwapChain(VkSwapchainKHR oldSwapChain) const
{
    const QueueFamilyIndices queueFamilies = findQueueFamilies(physicalDevice);
    if (!queueFamilies.isComplete())
    {
        throw std::runtime_error("failed to find queue families required by the swapchain!");
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    const VkExtent2D framebufferExtent = {
        static_cast<uint32_t>(std::max(width, 0)),
        static_cast<uint32_t>(std::max(height, 0))
    };

    return VulkanSwapchain(
        physicalDevice,
        device,
        surface,
        *queueFamilies.graphicsFamily,
        *queueFamilies.presentFamily,
        framebufferExtent,
        oldSwapChain);
}

void VulkanApp::createDepthResources()
{
    const VkFormat depthFormat = findDepthFormat();
    const VkExtent2D extent = swapChain.extent();

    createImage(
        extent.width,
        extent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory);

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanApp::createFramebuffers()
{
    const std::vector<VkImageView>& imageViews = swapChain.imageViews();
    const VkExtent2D extent = swapChain.extent();
    swapChainFramebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        const std::array<VkImageView, 2> attachments = { imageViews[i], depthImageView };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

}

void VulkanApp::requestSwapChainRecreation()
{
    swapChainRecreationRequested = true;
    lastFramebufferResizeTime = glfwGetTime();
}

bool VulkanApp::isSwapChainRecreationDue() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    return width > 0 && height > 0 &&
        glfwGetTime() - lastFramebufferResizeTime >= kSwapChainResizeDebounceSeconds;
}

void VulkanApp::cleanupSwapChainDependents()
{
    if (!commandBuffers.empty())
    {
        vkFreeCommandBuffers(
            device,
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data());
        commandBuffers.clear();
    }

    for (VkFramebuffer framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapChainFramebuffers.clear();

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, depthImageView, nullptr);
        depthImageView = VK_NULL_HANDLE;
    }
    if (depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }
    if (depthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, depthImageMemory, nullptr);
        depthImageMemory = VK_NULL_HANDLE;
    }

    if (graphicPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, graphicPipeline, nullptr);
        graphicPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    descriptorSets.clear();

    uniformBuffers.clear();

    imagesInFlight.clear();
}

void VulkanApp::cleanupSwapChain()
{
    cleanupSwapChainDependents();
    swapChain.reset();
}

void VulkanApp::recreateSwapChain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    vkDeviceWaitIdle(device);

    // Construct the replacement first. If creation fails, the current swapchain
    // and all of its dependent resources remain intact.
    VulkanSwapchain newSwapChain = makeSwapChain(swapChain.get());
    cleanupSwapChainDependents();
    swapChain = std::move(newSwapChain);

    // The projection matrix must use the new render extent; otherwise a resize
    // changes the viewport without changing the camera aspect ratio.
    const VkExtent2D extent = swapChain.extent();
    camera.setAspect(
        static_cast<float>(extent.width) /
        static_cast<float>(extent.height));

    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    imagesInFlight.assign(swapChain.imageCount(), VK_NULL_HANDLE);
    renderFinishedSemaphores.resize(swapChain.imageCount());
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t i = 0; i < swapChain.imageCount(); i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    swapChainRecreationRequested = false;
}
