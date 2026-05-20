#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

// ── GLM ───────────────────────────────────────────────────────────────────────
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ── GLFW ──────────────────────────────────────────────────────────────────────
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// ── Vulkan ────────────────────────────────────────────────────────────────────
#include <vulkan/vulkan.h>

// ─────────────────────────────────────────────────────────────────────────────

static void check_glm()
{
    glm::vec3 v(1.0f, 2.0f, 3.0f);
    glm::mat4 m = glm::translate(glm::mat4(1.0f), v);
    (void)m;
    std::cout << "[OK] GLM "
              << GLM_VERSION_MAJOR << '.'
              << GLM_VERSION_MINOR << '.'
              << GLM_VERSION_PATCH << '\n';
}

static void check_glfw()
{
    if (!glfwInit())
    {
        std::cerr << "[FAIL] glfwInit() failed\n";
        std::exit(EXIT_FAILURE);
    }
    int major, minor, rev;
    glfwGetVersion(&major, &minor, &rev);
    std::cout << "[OK] GLFW " << major << '.' << minor << '.' << rev << '\n';
    // Don't create a window — just verify the library is usable.
    glfwTerminate();
}

// static void check_vulkan()
// {
//     // Enumerate instance version (Vulkan 1.1+)
//     uint32_t apiVersion = 0;
//     if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS) {
//         std::cerr << "[FAIL] vkEnumerateInstanceVersion() failed\n";
//         std::exit(EXIT_FAILURE);
//     }
//     std::cout << "[OK] Vulkan instance version "
//               << VK_VERSION_MAJOR(apiVersion) << '.'
//               << VK_VERSION_MINOR(apiVersion) << '.'
//               << VK_VERSION_PATCH(apiVersion) << '\n';

//     // List available instance extensions
//     uint32_t extCount = 0;
//     vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
//     std::vector<VkExtensionProperties> exts(extCount);
//     vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());
//     std::cout << "     " << extCount << " instance extension(s) available\n";

//     std::vector<const char*> instanceExtensions;
//     uint32_t glfwExtensionCount = 0;
//     const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//     if (glfwExtensions != nullptr) {
//         instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
//     }

//     bool hasPortabilityEnumeration = false;
//     for (const auto& ext : exts) {
//         if (std::strcmp(ext.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
//             hasPortabilityEnumeration = true;
//             break;
//         }
//     }
//     if (hasPortabilityEnumeration) {
//         instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
//     }

//     // Create a minimal VkInstance to confirm loader + ICD work
//     VkApplicationInfo appInfo{};
//     appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//     appInfo.pApplicationName   = "VulkanCheck";
//     appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
//     appInfo.pEngineName        = "NoEngine";
//     appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
//     appInfo.apiVersion         = VK_API_VERSION_1_0;

//     VkInstanceCreateInfo createInfo{};
//     createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//     createInfo.pApplicationInfo      = &appInfo;
//     createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
//     createInfo.ppEnabledExtensionNames = instanceExtensions.empty() ? nullptr : instanceExtensions.data();
//     if (hasPortabilityEnumeration) {
//         createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
//     }

//     VkInstance instance = VK_NULL_HANDLE;
//     VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
//     if (result != VK_SUCCESS) {
//         std::cerr << "[FAIL] vkCreateInstance() -> VkResult " << result << '\n';
//         std::exit(EXIT_FAILURE);
//     }
//     std::cout << "[OK] VkInstance created successfully\n";

//     // List physical devices
//     uint32_t deviceCount = 0;
//     vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
//     std::vector<VkPhysicalDevice> devices(deviceCount);
//     vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
//     std::cout << "     " << deviceCount << " physical device(s) found\n";
//     for (const auto& dev : devices) {
//         VkPhysicalDeviceProperties props{};
//         vkGetPhysicalDeviceProperties(dev, &props);
//         std::cout << "       - " << props.deviceName
//                   << "  (API "
//                   << VK_VERSION_MAJOR(props.apiVersion) << '.'
//                   << VK_VERSION_MINOR(props.apiVersion) << '.'
//                   << VK_VERSION_PATCH(props.apiVersion) << ")\n";
//     }

//     vkDestroyInstance(instance, nullptr);
// }

// // ─────────────────────────────────────────────────────────────────────────────

const std::vector<const char*> validationLayers = {

    "VK_LAYER_KHRONOS_validation"

};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;

bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char *> getRequiredExtensions()
{
    uint32_t glfwExtensinCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensinCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensinCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cerr << "[Validation Layer] "
              << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo)
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

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
        );
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(
            instance,
            &createInfo,
            nullptr,
            &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger.");
    }
}

void createInstance(){
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance!");
    }
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "Vulkan Learning",
        nullptr,
        nullptr
    );

    createInstance();
    setupDebugMessenger();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}