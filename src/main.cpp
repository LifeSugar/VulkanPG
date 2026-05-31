#include <cstdlib>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanApp
{
public:
    void setPreferIntegratedGpu(bool enabled)
    {
        preferIntegratedGpu = enabled;
    }

    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // -------------------------
    // 基础窗口配置
    // -------------------------
    static constexpr uint32_t kWindowWidth = 1280;
    static constexpr uint32_t kWindowHeight = 720;

    // -------------------------
    // Vulkan 配置
    // -------------------------
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    // -------------------------
    // Vulkan 句柄（先放全，后面逐步填充）
    // -------------------------
    GLFWwindow *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // 逐步启用这些对象：
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{};
    static constexpr uint32_t kMaxFramesInFlight = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    
    bool preferIntegratedGpu = false;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

private:
    // -------------------------
    // 程序阶段函数
    // -------------------------
    void initWindow()
    {
        if (!glfwInit())
        {
            throw std::runtime_error("glfwInit() failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(
            static_cast<int>(kWindowWidth),
            static_cast<int>(kWindowHeight),
            "Vulkan Learning",
            nullptr,
            nullptr);

        if (window == nullptr)
        {
            throw std::runtime_error("glfwCreateWindow() failed");
        }
    }

    void initVulkan()
    {
        // 1) 已完成：创建实例 + 调试信使
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();

        // 2) 预留：后续逐步实现
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
    }

    void cleanup()
    {
        // 关闭前等待 GPU 空闲，避免资源还在使用时被销毁。
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
        }

        // 以后会按依赖顺序补齐更多销毁逻辑。
        for (VkImageView imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapChainImageViews.clear();

        for (VkFramebuffer framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        swapChainFramebuffers.clear();

        for (VkSemaphore semaphore : renderFinishedSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
        renderFinishedSemaphores.clear();

        for (VkSemaphore semaphore : imageAvailableSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
        imageAvailableSemaphores.clear();

        for (VkFence fence : inFlightFences)
        {
            vkDestroyFence(device, fence, nullptr);
        }
        inFlightFences.clear();

        if (graphicsPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }

        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
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

        if (swapChain != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }

        if (device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(device, nullptr);
        }

        if (debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }

        if (window != nullptr)
        {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        glfwTerminate();
    }

private:
    // -------------------------
    // 你当前已经掌握的模块
    // -------------------------
    bool checkValidationLayerSupport() const
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (std::strcmp(layerName, layerProperties.layerName) == 0)
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

    std::vector<const char *> getRequiredExtensions() const
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions;
        if (glfwExtensions != nullptr)
        {
            extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);
        }

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
        (void)messageSeverity;
        (void)messageType;
        (void)pUserData;

        std::cerr << "[Validation Layer] " << pCallbackData->pMessage << '\n';
        return VK_FALSE;
    }

    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
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

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance inInstance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(inInstance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            return func(inInstance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(
        VkInstance inInstance,
        VkDebugUtilsMessengerEXT inDebugMessenger,
        const VkAllocationCallbacks *pAllocator)
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(inInstance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            func(inInstance, inDebugMessenger, pAllocator);
        }
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
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

    void createInstance()
    {
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

        const std::vector<const char *> extensions = getRequiredExtensions();

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
#if defined(__APPLE__)
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
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

private:
    // -------------------------
    // 后续逐步实现的占位函数（先搭框架）
    // -------------------------
    static const char *deviceTypeToString(VkPhysicalDeviceType type)
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

    static std::string queueFlagsToString(VkQueueFlags flags)
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

        result.pop_back(); // Remove trailing '|'.
        return result;
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate, bool verbose = false) const
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, nullptr);

        if (verbose)
        {
            std::cout << "  [Queue] Family count = " << queueFamilyCount << '\n';
        }

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            const VkQueueFamilyProperties &queueFamily = queueFamilies[i];

            if (verbose)
            {
                std::cout << "    [Queue#" << i << "] flags="
                          << queueFlagsToString(queueFamily.queueFlags)
                          << ", queueCount=" << queueFamily.queueCount;
            }

            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && !indices.graphicsFamily.has_value())
            {
                indices.graphicsFamily = i;
                if (verbose)
                {
                    std::cout << " -> supports GRAPHICS";
                }
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);
            if (presentSupport == VK_TRUE && !indices.presentFamily.has_value())
            {
                indices.presentFamily = i;
                if (verbose)
                {
                    std::cout << " -> supports PRESENT";
                }
            }

            if (verbose)
            {
                std::cout << '\n';
            }

            if (indices.isComplete() && !verbose)
            {
                break;
            }
        }

        return indices;
    }

    bool isDeviceSuitable(VkPhysicalDevice candidate) const
    {
        const QueueFamilyIndices indices = findQueueFamilies(candidate);
        const bool extensionsSupported = checkDeviceExtensionSupport(candidate);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(candidate);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void pickPhysicalDevice()
    {
        std::cout << "[Step] Enumerating physical devices...\n";

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        std::cout << "[Step] Found " << deviceCount << " Vulkan-capable device(s).\n";

        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        auto deviceScore = [this](VkPhysicalDeviceType type) -> int
        {
            if (preferIntegratedGpu)
            {
                switch (type)
                {
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return 1000;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return 900;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return 200;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return 100;
                default:
                    return 50;
                }
            }

            switch (type)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return 1000;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return 900;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return 200;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return 100;
            default:
                return 50;
            }
        };

        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        int bestScore = -1;

        for (size_t deviceIndex = 0; deviceIndex < devices.size(); ++deviceIndex)
        {
            const VkPhysicalDevice candidate = devices[deviceIndex];

            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(candidate, &props);

            std::cout << "[Step] Checking device #" << deviceIndex << ": "
                      << props.deviceName
                      << " (" << deviceTypeToString(props.deviceType) << ")\n";

            const QueueFamilyIndices indices = findQueueFamilies(candidate, true);
            const bool suitable = isDeviceSuitable(candidate);

            if (suitable)
            {
                const int score = deviceScore(props.deviceType);
                std::cout << "[Step] Device accepted. graphicsFamily=" << *indices.graphicsFamily
                          << ", presentFamily=" << *indices.presentFamily
                          << ", score=" << score << "\n";

                if (score > bestScore)
                {
                    bestScore = score;
                    bestDevice = candidate;
                }

                continue;
            }

            std::cout << "[Step] Device rejected: not suitable for swapchain rendering.\n";
        }

        physicalDevice = bestDevice;

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        std::cout << "[OK] Selected GPU: " << props.deviceName << "\n";
    }

    void createLogicalDevice()
    {
        const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        if (!indices.isComplete())
        {
            throw std::runtime_error("Cannot create logical device: queue families are incomplete.");
        }

        std::vector<uint32_t> uniqueQueueFamilies;
        uniqueQueueFamilies.push_back(*indices.graphicsFamily);
        if (*indices.presentFamily != *indices.graphicsFamily)
        {
            uniqueQueueFamilies.push_back(*indices.presentFamily);
        }

        const float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilies.size());

        for (const uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        // Device layers are legacy and ignored by modern Vulkan loaders.
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, *indices.graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, *indices.presentFamily, 0, &presentQueue);

        std::cout << "[OK] Logical device created. graphicsQueueFamily=" << *indices.graphicsFamily
                  << ", presentQueueFamily=" << *indices.presentFamily << "\n";
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice candidate) const
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

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice candidate) const
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

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
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

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }

    void createSwapChain()
    {
        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        const uint32_t queueFamilyIndices[] = {*indices.graphicsFamily, *indices.presentFamily};

        if (*indices.graphicsFamily != *indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        std::cout << "[OK] Swapchain created. imageCount=" << imageCount
                  << ", extent=" << swapChainExtent.width << "x" << swapChainExtent.height
                  << "\n";
    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); ++i)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create swap chain image view!");
            }
        }

        std::cout << "[OK] Swapchain image views created. count=" << swapChainImageViews.size() << "\n";
    }

    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }

        std::cout << "[OK] Render pass created.\n";
    }

    static std::vector<char> readBinaryFile(const std::string &filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize <= 0)
        {
            throw std::runtime_error("Shader file is empty: " + filePath);
        }

        std::vector<char> buffer(static_cast<size_t>(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        if (!file)
        {
            throw std::runtime_error("Failed to read file: " + filePath);
        }

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char> &code) const
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    void createGraphicsPipeline()
    {
        if (renderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot create graphics pipeline: render pass has not been created yet.");
        }

        const std::vector<char> vertShaderCode = readBinaryFile("shaders/triangle.vert.spv");
        const std::vector<char> fragShaderCode = readBinaryFile("shaders/triangle.frag.spv");

        const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
            vertShaderStageInfo,
            fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        std::cout << "[OK] Graphics pipeline created (HLSL -> SPIR-V).\n";
    }

    void createFramebuffers()
    {
        if (swapChainImageViews.empty())
        {
            throw std::runtime_error("Cannot create framebuffers: swap chain image views are empty.");
        }

        if (swapChainImageViews.size() != swapChainImages.size())
        {
            throw std::runtime_error("Cannot create framebuffers: image view count does not match swap chain image count.");
        }

        if (renderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot create framebuffers: render pass has not been created yet.");
        }

        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); ++i)
        {
            VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create swap chain framebuffer!");
            }
        }

        std::cout << "[OK] Swapchain framebuffers created. count=" << swapChainFramebuffers.size() << "\n";
    }

    void createCommandPool()
    {
        const QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        if (!indices.graphicsFamily.has_value())
        {
            throw std::runtime_error("Cannot create command pool: graphics queue family is missing.");
        }

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = *indices.graphicsFamily;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }

        std::cout << "[OK] Command pool created. graphicsQueueFamily=" << *indices.graphicsFamily << "\n";
    }

    void createCommandBuffers()
    {
        if (commandPool == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot create command buffers: command pool has not been created yet.");
        }

        if (graphicsPipeline == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot create command buffers: graphics pipeline has not been created yet.");
        }

        if (renderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Cannot create command buffers: render pass has not been created yet.");
        }

        if (swapChainFramebuffers.empty())
        {
            throw std::runtime_error("Cannot create command buffers: framebuffers are empty.");
        }

        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); ++i)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            VkClearValue clearColor = {{{0.05f, 0.05f, 0.10f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to record command buffer!");
            }
        }

        std::cout << "[OK] Command buffers created. count=" << commandBuffers.size() << "\n";
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(kMaxFramesInFlight);
        renderFinishedSemaphores.resize(kMaxFramesInFlight);
        inFlightFences.resize(kMaxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }

        std::cout << "[OK] Sync objects created. framesInFlight=" << kMaxFramesInFlight << "\n";
    }

    void drawFrame()
    {
        if (commandBuffers.empty())
        {
            throw std::runtime_error("Cannot draw frame: command buffers are empty.");
        }

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        VkResult acquireResult = vkAcquireNextImageKHR(
            device,
            swapChain,
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            throw std::runtime_error("Swapchain is out of date. Resize handling is not implemented yet.");
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkSwapchainKHR swapChains[] = {swapChain};
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Swapchain is out of date/suboptimal. Resize handling is not implemented yet.");
        }
        if (presentResult != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    }
};

int main(int argc, char *argv[])
{
    VulkanApp app;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--integrated") == 0)
        {
            app.setPreferIntegratedGpu(true);
        }
        else if (std::strcmp(argv[i], "--discrete") == 0)
        {
            app.setPreferIntegratedGpu(false);
        }
    }

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}