#include "VKApp.h"
#include <iostream>
#include <set>
#include <stdexcept>

const char* VulkanApp::PhysicalDeviceTypeToString(VkPhysicalDeviceType type)
{
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "Integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "Discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU";
    default:
        return "Other";
    }
}

std::string VulkanApp::queueFamilyIndicesString(VkQueueFlags flags)
{
    std::string result;
    if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0)
    {
        result += "GRAPHICS|";
    }
    if ((flags & VK_QUEUE_COMPUTE_BIT) != 0)
    {
        result += "COMPUTE|";
    }
    if ((flags & VK_QUEUE_TRANSFER_BIT) != 0)
    {
        result += "TRANSFER|";
    }
    if ((flags & VK_QUEUE_SPARSE_BINDING_BIT) != 0)
    {
        result += "SPARSE_BINDING|";
    }
    if (result.empty())
    {
        return "NONE";
    }
    result.pop_back(); // remove trailing '|'
    return result;
}

VulkanApp::QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice candidate) const
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        const VkQueueFamilyProperties &queueFamily = queueFamilies[i];
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
    }

    return indices;
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice candidate) const 
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, availableExtensions.data());
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

VulkanApp::SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice candidate) const
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(candidate, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(candidate, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0; 
    vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(candidate, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

bool VulkanApp::isDeviceSuitable(VkPhysicalDevice candidate) const
{
    const QueueFamilyIndices indices = findQueueFamilies(candidate);
    const bool extensionsSupported = checkDeviceExtensionSupport(candidate);
    bool swapchainAdequate = false;
    if (extensionsSupported)
    {
        const SwapChainSupportDetails support = querySwapChainSupport(candidate);
        swapchainAdequate = !support.formats.empty() && !support.presentModes.empty();
    }
    return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

void VulkanApp::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    int bestScore = -1;
    for (const auto &device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "Checking device: " << deviceProperties.deviceName
                    << " (" << PhysicalDeviceTypeToString(deviceProperties.deviceType) << ")\n";
        if (!isDeviceSuitable(device))
        {
            std::cout << " -> Device is not suitable.\n";
            continue;
        }

        auto deviceScore = [this](VkPhysicalDeviceType type) -> int
        {
                switch (type)
                {
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return 1000;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return 500;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return 100;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return 50;
                default:
                    return 0;
                }
        };

        if (preferIntegratedGpu && deviceScore(deviceProperties.deviceType) == 500)
        {
            bestDevice = device;
            break;
        }
        if (deviceScore(deviceProperties.deviceType) > bestScore)
        {
            bestScore = deviceScore(deviceProperties.deviceType);
            bestDevice = device;
        }
    }
    physicalDevice = bestDevice;
    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    swapChainSupport = querySwapChainSupport(physicalDevice);
}

void VulkanApp::createLogicalDevice()
{
    const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    if (!indices.isComplete())
    {
        throw std::runtime_error("failed to find required queue families!");
    }

    std::vector<uint32_t> uniqueQueueFamilies;
    uniqueQueueFamilies.push_back(*indices.graphicsFamily);
    if (*indices.presentFamily != *indices.graphicsFamily)
    {
        uniqueQueueFamilies.push_back(*indices.presentFamily);
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        const float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    // Device layers deprecated since Vulkan 1.0; validation is handled at instance level.
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, *indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, *indices.presentFamily,  0, &presentQueue);
}

