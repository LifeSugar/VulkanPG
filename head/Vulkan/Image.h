#pragma once

#include "Vulkan/VulkanMemoryConfig.h"

#include <vulkan/vulkan.h>
#if VK_RENDERER_USE_VMA
#include <vk_mem_alloc.h>
#endif

namespace VkRenderer
{

class Device;

/// RAII wrapper for a Vulkan image and its bound memory.
class Image final
{
public:
    /// Creates an empty image wrapper.
    Image() = default;
    /// Creates a 2D image and allocates matching device memory.
    Image(
        const Device& device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    /// Destroys the owned image and memory.
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    /// Transfers image and memory ownership from another wrapper.
    Image(Image&& other) noexcept;
    /// Replaces this image by taking ownership from another wrapper.
    Image& operator=(Image&& other) noexcept;

    /// Creates or replaces the image and its backing memory.
    void create(
        const Device& device,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    /// Releases the owned image and memory.
    void reset() noexcept;

    /// Returns the owned Vulkan image handle.
    [[nodiscard]] VkImage get() const noexcept { return image_; }
    /// Returns the memory bound to the image.
    [[nodiscard]] VkDeviceMemory memory() const noexcept;
    /// Returns whether an image is currently owned.
    [[nodiscard]] explicit operator bool() const noexcept { return image_ != VK_NULL_HANDLE; }

private:
#if VK_RENDERER_USE_VMA
    /// Allocator that owns the image allocation.
    VmaAllocator allocator_ = VK_NULL_HANDLE;
#else
    /// Logical device that owns the image and memory.
    VkDevice device_ = VK_NULL_HANDLE;
#endif
    /// Owned Vulkan image handle.
    VkImage image_ = VK_NULL_HANDLE;
#if VK_RENDERER_USE_VMA
    /// Device memory bound to the image.
    VmaAllocation allocation_ = VK_NULL_HANDLE;
#else
    /// Original Vulkan allocation bound to the image.
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
#endif
};

} // namespace VkRenderer
