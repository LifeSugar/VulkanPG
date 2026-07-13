#pragma once

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>

class VulkanSwapchain final
{
public:
    struct SupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    VulkanSwapchain() = default;
    VulkanSwapchain(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkSurfaceKHR surface,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkExtent2D framebufferExtent,
        VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VulkanSwapchain(VulkanSwapchain&& other) noexcept;
    VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

    void create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkSurfaceKHR surface,
        uint32_t graphicsQueueFamily,
        uint32_t presentQueueFamily,
        VkExtent2D framebufferExtent,
        VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void reset() noexcept;

    [[nodiscard]] VkSwapchainKHR get() const noexcept { return swapchain_; }
    [[nodiscard]] VkFormat format() const noexcept { return format_; }
    [[nodiscard]] VkExtent2D extent() const noexcept { return extent_; }
    [[nodiscard]] std::size_t imageCount() const noexcept { return images_.size(); }
    [[nodiscard]] const std::vector<VkImage>& images() const noexcept { return images_; }
    [[nodiscard]] const std::vector<VkImageView>& imageViews() const noexcept { return imageViews_; }
    [[nodiscard]] explicit operator bool() const noexcept { return swapchain_ != VK_NULL_HANDLE; }

    [[nodiscard]] static SupportDetails querySupport(
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface);

private:
    [[nodiscard]] static VkSurfaceFormatKHR chooseSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    [[nodiscard]] static VkPresentModeKHR choosePresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    [[nodiscard]] static VkExtent2D chooseExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        VkExtent2D framebufferExtent);

    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
};
