#pragma once

#include <vulkan/vulkan.h>

namespace VkRenderer
{

class Image final
{
public:
    Image() = default;
    Image(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    void create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties);
    void reset() noexcept;

    [[nodiscard]] VkImage get() const noexcept { return image_; }
    [[nodiscard]] VkDeviceMemory memory() const noexcept { return memory_; }
    [[nodiscard]] explicit operator bool() const noexcept { return image_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
