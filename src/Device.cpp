#include "Device.h"

#include "Swapchain.h"

#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{
namespace
{

constexpr std::array<const char*, 1> kRequiredExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

} // namespace

Device::Device(
    VkInstance instance,
    VkSurfaceKHR surface,
    bool preferIntegratedGpu)
{
    create(instance, surface, preferIntegratedGpu);
}

Device::~Device()
{
    reset();
}

Device::Device(Device&& other) noexcept
    : physicalDevice_(std::exchange(other.physicalDevice_, VK_NULL_HANDLE)),
      device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      graphicsQueue_(std::exchange(other.graphicsQueue_, VK_NULL_HANDLE)),
      presentQueue_(std::exchange(other.presentQueue_, VK_NULL_HANDLE)),
      graphicsQueueFamily_(std::exchange(other.graphicsQueueFamily_, 0)),
      presentQueueFamily_(std::exchange(other.presentQueueFamily_, 0))
{
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this != &other)
    {
        reset();
        physicalDevice_ = std::exchange(other.physicalDevice_, VK_NULL_HANDLE);
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        graphicsQueue_ = std::exchange(other.graphicsQueue_, VK_NULL_HANDLE);
        presentQueue_ = std::exchange(other.presentQueue_, VK_NULL_HANDLE);
        graphicsQueueFamily_ = std::exchange(other.graphicsQueueFamily_, 0);
        presentQueueFamily_ = std::exchange(other.presentQueueFamily_, 0);
    }
    return *this;
}

void Device::create(
    VkInstance instance,
    VkSurfaceKHR surface,
    bool preferIntegratedGpu)
{
    if (instance == VK_NULL_HANDLE || surface == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("cannot create a Device with an invalid instance or surface");
    }

    uint32_t physicalDeviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS ||
        physicalDeviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to enumerate Vulkan physical devices!");
    }

    VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilies;
    int bestScore = -1;

    for (VkPhysicalDevice candidate : physicalDevices)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        std::cout << "Checking device: " << properties.deviceName
                  << " (" << physicalDeviceTypeName(properties.deviceType) << ")\n";

        if (!isSuitable(candidate, surface))
        {
            std::cout << " -> Device is not suitable.\n";
            continue;
        }

        int score = 0;
        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = 500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = 100;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score = 50;
            break;
        default:
            break;
        }

        // Preserve the application's existing selection policy.
        if (preferIntegratedGpu && score == 500)
        {
            selectedPhysicalDevice = candidate;
            selectedQueueFamilies = findQueueFamilies(candidate, surface);
            break;
        }

        if (score > bestScore)
        {
            bestScore = score;
            selectedPhysicalDevice = candidate;
            selectedQueueFamilies = findQueueFamilies(candidate, surface);
        }
    }

    if (selectedPhysicalDevice == VK_NULL_HANDLE || !selectedQueueFamilies.complete())
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    const std::array<uint32_t, 2> queueFamilyCandidates = {
        *selectedQueueFamilies.graphics,
        *selectedQueueFamilies.present
    };
    std::set<uint32_t> uniqueQueueFamilies(
        queueFamilyCandidates.begin(),
        queueFamilyCandidates.end());

    constexpr float kQueuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &kQueuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(kRequiredExtensions.size());
    createInfo.ppEnabledExtensionNames = kRequiredExtensions.data();
    createInfo.pEnabledFeatures = &features;

    VkDevice newDevice = VK_NULL_HANDLE;
    if (vkCreateDevice(selectedPhysicalDevice, &createInfo, nullptr, &newDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    VkQueue newGraphicsQueue = VK_NULL_HANDLE;
    VkQueue newPresentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(newDevice, *selectedQueueFamilies.graphics, 0, &newGraphicsQueue);
    vkGetDeviceQueue(newDevice, *selectedQueueFamilies.present, 0, &newPresentQueue);

    reset();
    physicalDevice_ = selectedPhysicalDevice;
    device_ = newDevice;
    graphicsQueue_ = newGraphicsQueue;
    presentQueue_ = newPresentQueue;
    graphicsQueueFamily_ = *selectedQueueFamilies.graphics;
    presentQueueFamily_ = *selectedQueueFamilies.present;
}

void Device::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device_, nullptr);
    }

    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    graphicsQueue_ = VK_NULL_HANDLE;
    presentQueue_ = VK_NULL_HANDLE;
    graphicsQueueFamily_ = 0;
    presentQueueFamily_ = 0;
}

void Device::waitIdle() const
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);
    }
}

Device::QueueFamilyIndices Device::findQueueFamilies(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            indices.graphics = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport == VK_TRUE)
        {
            indices.present = i;
        }

        if (indices.complete())
        {
            break;
        }
    }

    return indices;
}

bool Device::supportsRequiredExtensions(VkPhysicalDevice physicalDevice)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &extensionCount,
        availableExtensions.data());

    std::set<std::string> requiredExtensions(
        kRequiredExtensions.begin(),
        kRequiredExtensions.end());
    for (const VkExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

bool Device::isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    const QueueFamilyIndices queueFamilies = findQueueFamilies(physicalDevice, surface);
    if (!queueFamilies.complete() || !supportsRequiredExtensions(physicalDevice))
    {
        return false;
    }

    const Swapchain::SupportDetails swapchainSupport =
        Swapchain::querySupport(physicalDevice, surface);
    return !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
}

const char* Device::physicalDeviceTypeName(VkPhysicalDeviceType type) noexcept
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

std::string Device::queueFlagsName(VkQueueFlags flags)
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
    result.pop_back();
    return result;
}

} // namespace VkRenderer
