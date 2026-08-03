#pragma once

#include "Device.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace VkRenderer
{

class Window;

/// Owns the Vulkan instance, surface, device, and optional debug messenger.
class VulkanContext final
{
public:
    /// Parameters used to initialize the Vulkan instance and device.
    struct CreateInfo
    {
        /// Application name reported to the Vulkan driver.
        std::string applicationName = "Hello Vulkan";
        /// Application version reported to the Vulkan driver.
        uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        /// Engine name reported to the Vulkan driver.
        std::string engineName = "No Engine";
        /// Engine version reported to the Vulkan driver.
        uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
        /// Highest Vulkan API version requested by the application.
        uint32_t apiVersion = VK_API_VERSION_1_3;
        /// Whether validation layers and debug messages are enabled.
        bool enableValidationLayers = false;
        /// Validation layers requested when validation is enabled.
        std::vector<std::string> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        /// Whether physical-device selection should prefer an integrated GPU.
        bool preferIntegratedGpu = false;
    };

    /// Creates an empty Vulkan context.
    VulkanContext() = default;
    /// Creates a Vulkan context for the supplied window.
#ifndef __ANDROID__
    VulkanContext(const Window& window, const CreateInfo& createInfo);
#endif
    /// Releases the device, surface, debug messenger, and instance.
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    /// Transfers ownership from another context.
    VulkanContext(VulkanContext&& other) noexcept;
    /// Replaces this context by taking ownership from another context.
    VulkanContext& operator=(VulkanContext&& other) noexcept;

    /// Creates or replaces all context-level Vulkan resources.
#ifndef __ANDROID__
    void create(const Window& window, const CreateInfo& createInfo);
#endif
    /// Creates or replaces context for an externally managed surface (Android / headless).
    void create(VkSurfaceKHR surface, const CreateInfo& createInfo);
    /// Creates the Vulkan instance and optional debug messenger (Android two-phase init).
    void initInstance(const CreateInfo& createInfo);
    /// Creates the device for an externally created surface (Android two-phase init).
    void initSurfaceAndDevice(VkSurfaceKHR surface, bool preferIntegratedGpu);
    /// Releases all owned Vulkan resources.
    void reset() noexcept;
    /// Releases Vulkan resources without destroying the surface (Android lifecycle).
    void resetWithoutSurface() noexcept;
    /// Waits until the logical device has no pending work.
    void waitIdle() const;

    /// Returns the owned Vulkan instance.
    [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
    /// Returns the window presentation surface.
    [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }
    /// Returns mutable access to the selected logical device.
    [[nodiscard]] Device& device() noexcept { return device_; }
    /// Returns read-only access to the selected logical device.
    [[nodiscard]] const Device& device() const noexcept { return device_; }
    /// Returns whether validation layers are active.
    [[nodiscard]] bool validationLayersEnabled() const noexcept
    {
        return validationLayersEnabled_;
    }
    /// Returns whether the context is fully initialized.
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    /// Checks that every requested validation layer is available.
    [[nodiscard]] static bool checkValidationLayerSupport(
        const std::vector<std::string>& validationLayers);
    /// Builds the Vulkan instance extension list for the selected options.
    [[nodiscard]] static std::vector<const char*> requiredExtensions(
        bool enableValidationLayers);
    /// Writes validation messages to the diagnostic stream.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);
    /// Fills the standard debug-messenger creation parameters.
    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    /// Creates a debug messenger through the extension entry point.
    [[nodiscard]] static VkDebugUtilsMessengerEXT createDebugMessenger(
        VkInstance instance);
    /// Destroys a debug messenger through the extension entry point.
    static void destroyDebugMessenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger) noexcept;

    /// Common Vulkan instance + device creation shared by all create() paths.
    void createInternal(
        const std::vector<const char*>& extensions,
        std::function<VkSurfaceKHR(VkInstance)> surfaceFactory,
        const CreateInfo& createInfo);

    /// Root Vulkan instance owned by this context.
    VkInstance instance_ = VK_NULL_HANDLE;
    /// Optional validation-layer debug messenger.
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    /// Presentation surface associated with the window.
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    /// Selected physical device, logical device, queues, and queue families.
    Device device_;
    /// Whether validation support was enabled at creation time.
    bool validationLayersEnabled_ = false;
};

} // namespace VkRenderer
