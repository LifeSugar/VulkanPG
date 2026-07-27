#pragma once

#include "Device.h"

#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

class Window;

class VulkanContext final
{
public:
    struct CreateInfo
    {
        std::string applicationName = "Hello Vulkan";
        uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        std::string engineName = "No Engine";
        uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
        uint32_t apiVersion = VK_API_VERSION_1_3;
        bool enableValidationLayers = false;
        std::vector<std::string> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        bool preferIntegratedGpu = false;
    };

    VulkanContext() = default;
    VulkanContext(const Window& window, const CreateInfo& createInfo);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VulkanContext(VulkanContext&& other) noexcept;
    VulkanContext& operator=(VulkanContext&& other) noexcept;

    void create(const Window& window, const CreateInfo& createInfo);
    void reset() noexcept;
    void waitIdle() const;

    [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
    [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }
    [[nodiscard]] Device& device() noexcept { return device_; }
    [[nodiscard]] const Device& device() const noexcept { return device_; }
    [[nodiscard]] bool validationLayersEnabled() const noexcept
    {
        return validationLayersEnabled_;
    }
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    [[nodiscard]] static bool checkValidationLayerSupport(
        const std::vector<std::string>& validationLayers);
    [[nodiscard]] static std::vector<const char*> requiredExtensions(
        bool enableValidationLayers);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);
    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    [[nodiscard]] static VkDebugUtilsMessengerEXT createDebugMessenger(
        VkInstance instance);
    static void destroyDebugMessenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger) noexcept;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    Device device_;
    bool validationLayersEnabled_ = false;
};

} // namespace VkRenderer
