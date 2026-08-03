#include "VulkanContext.h"

#ifndef __ANDROID__
#include "Window.h"
#endif

#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{

#ifndef __ANDROID__
VulkanContext::VulkanContext(
    const Window& window,
    const CreateInfo& createInfo)
{
    create(window, createInfo);
}
#endif

VulkanContext::~VulkanContext()
{
    reset();
}

VulkanContext::VulkanContext(VulkanContext&& other) noexcept
    : instance_(std::exchange(other.instance_, VK_NULL_HANDLE)),
      debugMessenger_(std::exchange(other.debugMessenger_, VK_NULL_HANDLE)),
      surface_(std::exchange(other.surface_, VK_NULL_HANDLE)),
      device_(std::move(other.device_)),
      validationLayersEnabled_(
          std::exchange(other.validationLayersEnabled_, false))
{
}

VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept
{
    if (this != &other)
    {
        reset();
        instance_ = std::exchange(other.instance_, VK_NULL_HANDLE);
        debugMessenger_ =
            std::exchange(other.debugMessenger_, VK_NULL_HANDLE);
        surface_ = std::exchange(other.surface_, VK_NULL_HANDLE);
        device_ = std::move(other.device_);
        validationLayersEnabled_ =
            std::exchange(other.validationLayersEnabled_, false);
    }
    return *this;
}

#ifndef __ANDROID__
void VulkanContext::create(
    const Window& window,
    const CreateInfo& createInfo)
{
    if (!window)
    {
        throw std::invalid_argument(
            "cannot create a VulkanContext without a window");
    }

    const std::vector<const char*> extensions =
        requiredExtensions(createInfo.enableValidationLayers);

    createInternal(
        extensions,
        [&window](VkInstance instance)
        {
            return window.createVulkanSurface(instance);
        },
        createInfo);
}
#endif

void VulkanContext::create(
    VkSurfaceKHR surface,
    const CreateInfo& createInfo)
{
    if (surface == VK_NULL_HANDLE)
    {
        throw std::invalid_argument(
            "cannot create a VulkanContext with a null surface");
    }

    const std::vector<const char*> extensions =
        requiredExtensions(createInfo.enableValidationLayers);

    createInternal(
        extensions,
        [surface](VkInstance /*instance*/)
        {
            return surface;
        },
        createInfo);
}

void VulkanContext::createInternal(
    const std::vector<const char*>& extensions,
    std::function<VkSurfaceKHR(VkInstance)> surfaceFactory,
    const CreateInfo& createInfo)
{
    if (createInfo.enableValidationLayers &&
        !checkValidationLayerSupport(createInfo.validationLayers))
    {
        throw std::runtime_error(
            "validation layers requested, but not available");
    }

    VulkanContext replacement;

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = createInfo.applicationName.c_str();
    applicationInfo.applicationVersion = createInfo.applicationVersion;
    applicationInfo.pEngineName = createInfo.engineName.c_str();
    applicationInfo.engineVersion = createInfo.engineVersion;
    applicationInfo.apiVersion = createInfo.apiVersion;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
#if defined(__APPLE__)
    instanceCreateInfo.flags |=
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    std::vector<const char*> validationLayerNames;
    if (createInfo.enableValidationLayers)
    {
        validationLayerNames.reserve(createInfo.validationLayers.size());
        for (const std::string& layerName : createInfo.validationLayers)
        {
            validationLayerNames.push_back(layerName.c_str());
        }
        instanceCreateInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayerNames.size());
        instanceCreateInfo.ppEnabledLayerNames =
            validationLayerNames.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = &debugCreateInfo;
    }

    if (vkCreateInstance(
            &instanceCreateInfo,
            nullptr,
            &replacement.instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create Vulkan instance");
    }

    if (createInfo.enableValidationLayers)
    {
        replacement.debugMessenger_ =
            createDebugMessenger(replacement.instance_);
    }

    replacement.surface_ = surfaceFactory(replacement.instance_);
    replacement.device_.create(
        replacement.instance_,
        replacement.surface_,
        createInfo.preferIntegratedGpu);
    replacement.validationLayersEnabled_ =
        createInfo.enableValidationLayers;

    *this = std::move(replacement);
}

void VulkanContext::initInstance(const CreateInfo& createInfo)
{
    if (createInfo.enableValidationLayers &&
        !checkValidationLayerSupport(createInfo.validationLayers))
    {
        throw std::runtime_error(
            "validation layers requested, but not available");
    }

    // Clean up any previous instance before creating a new one.
    if (debugMessenger_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE)
    {
        destroyDebugMessenger(instance_, debugMessenger_);
    }
    debugMessenger_ = VK_NULL_HANDLE;

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
    }
    instance_ = VK_NULL_HANDLE;
    surface_ = VK_NULL_HANDLE;
    validationLayersEnabled_ = false;

    const std::vector<const char*> extensions =
        requiredExtensions(createInfo.enableValidationLayers);

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = createInfo.applicationName.c_str();
    applicationInfo.applicationVersion = createInfo.applicationVersion;
    applicationInfo.pEngineName = createInfo.engineName.c_str();
    applicationInfo.engineVersion = createInfo.engineVersion;
    applicationInfo.apiVersion = createInfo.apiVersion;

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
#if defined(__APPLE__)
    instanceCreateInfo.flags |=
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    std::vector<const char*> validationLayerNames;
    if (createInfo.enableValidationLayers)
    {
        validationLayerNames.reserve(createInfo.validationLayers.size());
        for (const std::string& layerName : createInfo.validationLayers)
        {
            validationLayerNames.push_back(layerName.c_str());
        }
        instanceCreateInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayerNames.size());
        instanceCreateInfo.ppEnabledLayerNames =
            validationLayerNames.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCreateInfo.pNext = &debugCreateInfo;
    }

    if (vkCreateInstance(
            &instanceCreateInfo,
            nullptr,
            &instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create Vulkan instance");
    }

    if (createInfo.enableValidationLayers)
    {
        debugMessenger_ = createDebugMessenger(instance_);
    }

    validationLayersEnabled_ = createInfo.enableValidationLayers;
}

void VulkanContext::initSurfaceAndDevice(
    VkSurfaceKHR surface,
    bool preferIntegratedGpu)
{
    if (surface == VK_NULL_HANDLE)
    {
        throw std::invalid_argument("surface must not be null");
    }
    if (instance_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error(
            "initInstance must be called before initSurfaceAndDevice");
    }

    surface_ = surface;
    device_.create(instance_, surface_, preferIntegratedGpu);
}

void VulkanContext::reset() noexcept
{
    device_.waitIdle();
    device_.reset();

    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
    surface_ = VK_NULL_HANDLE;

    if (debugMessenger_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE)
    {
        destroyDebugMessenger(instance_, debugMessenger_);
    }
    debugMessenger_ = VK_NULL_HANDLE;

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
    }
    instance_ = VK_NULL_HANDLE;
    validationLayersEnabled_ = false;
}

void VulkanContext::resetWithoutSurface() noexcept
{
    device_.waitIdle();
    device_.reset();

    // Surface is externally managed (Android), do not destroy it.
    surface_ = VK_NULL_HANDLE;

    if (debugMessenger_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE)
    {
        destroyDebugMessenger(instance_, debugMessenger_);
    }
    debugMessenger_ = VK_NULL_HANDLE;

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
    }
    instance_ = VK_NULL_HANDLE;
    validationLayersEnabled_ = false;
}

void VulkanContext::waitIdle() const
{
    device_.waitIdle();
}

VulkanContext::operator bool() const noexcept
{
    return instance_ != VK_NULL_HANDLE &&
        surface_ != VK_NULL_HANDLE &&
        static_cast<bool>(device_);
}

bool VulkanContext::checkValidationLayerSupport(
    const std::vector<std::string>& validationLayers)
{
    uint32_t layerCount = 0;
    if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);
    if (vkEnumerateInstanceLayerProperties(
            &layerCount,
            availableLayers.data()) != VK_SUCCESS)
    {
        return false;
    }

    for (const std::string& layerName : validationLayers)
    {
        bool found = false;
        for (const VkLayerProperties& layer : availableLayers)
        {
            if (std::strcmp(layerName.c_str(), layer.layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

std::vector<const char*> VulkanContext::requiredExtensions(
    bool enableValidationLayers)
{
    std::vector<const char*> extensions;
#if defined(__ANDROID__)
    extensions.push_back("VK_KHR_android_surface");
    extensions.push_back("VK_KHR_surface");
#else
    extensions = Window::requiredVulkanInstanceExtensions();
#endif
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#if defined(__APPLE__)
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*)
{
    const char* severity = "unknown";
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severity = "verbose";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severity = "info";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity = "warning";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity = "error";
        break;
    default:
        break;
    }

    std::cerr << "validation layer (" << severity << "): "
              << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

void VulkanContext::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

VkDebugUtilsMessengerEXT VulkanContext::createDebugMessenger(
    VkInstance instance)
{
    const auto createFunction =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                instance,
                "vkCreateDebugUtilsMessengerEXT"));
    if (createFunction == nullptr)
    {
        throw std::runtime_error(
            "Vulkan debug utils extension is not available");
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (createFunction(
            instance,
            &createInfo,
            nullptr,
            &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create Vulkan debug messenger");
    }
    return debugMessenger;
}

void VulkanContext::destroyDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger) noexcept
{
    const auto destroyFunction =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                instance,
                "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyFunction != nullptr)
    {
        destroyFunction(instance, debugMessenger, nullptr);
    }
}

} // namespace VkRenderer
