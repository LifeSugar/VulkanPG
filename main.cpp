#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanApp
{
public:
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

    // 后续步骤会逐步启用这些对象：
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;

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

        // 2) 预留：后续逐步实现
        // createLogicalDevice();
        // createSwapChain();
        // createImageViews();
        // createRenderPass();
        // createGraphicsPipeline();
        // createFramebuffers();
        // createCommandPool();
        // createCommandBuffers();
        // createSyncObjects();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // 后续替换成 drawFrame()：
            // drawFrame();
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
    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidate) const
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

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &presentSupport);
            if (presentSupport == VK_TRUE)
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

    bool isDeviceSuitable(VkPhysicalDevice candidate) const
    {
        const QueueFamilyIndices indices = findQueueFamilies(candidate);
        return indices.isComplete();
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const VkPhysicalDevice &candidate : devices)
        {
            if (isDeviceSuitable(candidate))
            {
                physicalDevice = candidate;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        std::cout << "[OK] Selected GPU: " << props.deviceName << '\n';
    }

    void createLogicalDevice()
    {
        // TODO: 创建逻辑设备并拿到 graphicsQueue/presentQueue。
    }

    void createSwapChain()
    {
        // TODO: 选择 surface format / present mode / extent 并创建 swapchain。
    }

    void createImageViews()
    {
        // TODO: 给 swapchain 每张 image 创建 image view。
    }

    void createRenderPass()
    {
        // TODO: 先做单颜色附件 render pass（清屏 + 输出到屏幕）。
    }

    void createGraphicsPipeline()
    {
        // TODO: 加载 SPIR-V，创建 pipeline layout + graphics pipeline。
    }

    void createFramebuffers()
    {
        // TODO: 每个 swapchain image view 对应一个 framebuffer。
    }

    void createCommandPool()
    {
        // TODO: 为 graphics queue family 创建 command pool。
    }

    void createCommandBuffers()
    {
        // TODO: 录制 render pass + bind pipeline + vkCmdDraw(3,1,0,0)。
    }

    void createSyncObjects()
    {
        // TODO: 创建信号量和栅栏，协调 acquire/submit/present。
    }

    void drawFrame()
    {
        // TODO: acquire image -> submit command buffer -> present。
    }
};

int main()
{
    VulkanApp app;

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