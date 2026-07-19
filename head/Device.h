#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <string>

namespace VkRenderer
{

class Device final
{
public:
    Device() = default;
    Device(
        VkInstance instance,
        VkSurfaceKHR surface,
        bool preferIntegratedGpu = false);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    void create(
        VkInstance instance,
        VkSurfaceKHR surface,
        bool preferIntegratedGpu = false);
    void reset() noexcept;
    void waitIdle() const;

    [[nodiscard]] VkPhysicalDevice physical() const noexcept { return physicalDevice_; }
    [[nodiscard]] VkDevice get() const noexcept { return device_; }
    [[nodiscard]] VkQueue graphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] VkQueue presentQueue() const noexcept { return presentQueue_; }
    [[nodiscard]] uint32_t graphicsQueueFamily() const noexcept { return graphicsQueueFamily_; }
    [[nodiscard]] uint32_t presentQueueFamily() const noexcept { return presentQueueFamily_; }
    [[nodiscard]] explicit operator bool() const noexcept { return device_ != VK_NULL_HANDLE; }

private:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        [[nodiscard]] bool complete() const noexcept
        {
            return graphics.has_value() && present.has_value();
        }
    };

    [[nodiscard]] static QueueFamilyIndices findQueueFamilies(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface);
    [[nodiscard]] static bool supportsRequiredExtensions(VkPhysicalDevice physicalDevice);
    [[nodiscard]] static bool isSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    [[nodiscard]] static const char* physicalDeviceTypeName(VkPhysicalDeviceType type) noexcept;
    [[nodiscard]] static std::string queueFlagsName(VkQueueFlags flags);

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;
    uint32_t presentQueueFamily_ = 0;
};

} // namespace VkRenderer
