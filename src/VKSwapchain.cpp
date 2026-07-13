#include "VKApp.h"
#include <algorithm>
#include <array>
#include <stdexcept>

VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void VulkanApp::createSwapChain(VkSwapchainKHR oldSwapChain)
{
    swapChainSupport = querySwapChainSupport(physicalDevice);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
    {
        throw std::runtime_error("swap chain support is no longer adequate!");
    }

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    swapChainImageFormat = surfaceFormat.format;
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    auto imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.flags = 0;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainImageFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    swapChainExtent = extent;
}

void VulkanApp::createImageViews()
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanApp::createDepthResources()
{
    const VkFormat depthFormat = findDepthFormat();

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
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
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        const std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageView };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
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

void VulkanApp::cleanupSwapChain()
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

    for (size_t i = 0; i < uniformBuffers.size(); ++i)
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    uniformBuffers.clear();
    uniformBuffersMemory.clear();

    for (VkImageView imageView : swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainImageViews.clear();
    swapChainImages.clear();
    imagesInFlight.clear();

    if (swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
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

    // Keep the old handle alive while the new Swapchain is created, allowing the
    // implementation to recycle its resources.  Its dependent resources are no
    // longer needed once the device is idle.
    const VkSwapchainKHR oldSwapChain = swapChain;
    swapChain = VK_NULL_HANDLE;
    cleanupSwapChain();

    createSwapChain(oldSwapChain);
    if (oldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
    }

    // The projection matrix must use the new render extent; otherwise a resize
    // changes the viewport without changing the camera aspect ratio.
    camera.setAspect(
        static_cast<float>(swapChainExtent.width) /
        static_cast<float>(swapChainExtent.height));

    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    imagesInFlight.assign(swapChainImages.size(), VK_NULL_HANDLE);

    swapChainRecreationRequested = false;
}

